#include <cmath>
#include <algorithm>
#include <functional>
#include <chrono>

#include "ClientSender.h"

#include "Timer.h"


ClientSender::ClientSender(int chunkSize, ClientSubject *clientSubject) :
        dataProcessor(clientSubject->getFilename(), chunkSize),
        clientSubject(clientSubject),

        base_idx(0),
        next_seq_num_idx(0),
        window_size(clientSubject->getWindow_size()),
        packet_size(clientSubject->getPacket_size()),
        server_port(std::to_string(clientSubject->getServer_port())),
        server_host(clientSubject->getServer_host()),
        max_seq_number(clientSubject->getMax_seq_number()),
        max_seq_num_b10((int) (std::pow(10, max_seq_number) - 1)),
        socket(io_service, udp::endpoint(udp::v4(), 0)),
        resolver(io_service),
        query(udp::v4(), server_host, server_port),
        qiterator(resolver.resolve(query)),
        endpoint(*qiterator),
        window_map(max_seq_num_b10 + 1),
        acked_count(0),
        last_acked_packet(-1),
        max_seq_num_sent(-1),
        finished_sending_data(false) {
  asio::socket_base::send_buffer_size option(packet_size + 32 + max_seq_number + 256);
  socket.set_option(option);

}

void ClientSender::start() {
  sender_thread = std::make_unique<std::thread>(&ClientSender::threadFun, this);
}

ClientSender::~ClientSender() {
  if (sender_thread && sender_thread->joinable()) {
    std::cout << "joining client sender" << std::endl;
    sender_thread->join();
  } else {
    std::cout << "Couldn't join ClientSender" << std::endl;
  }
  socket.close();
}


Packet ClientSender::generatePacket(const std::string &raw_data, int packet_seq_num) {
  return Packet(max_seq_number, packet_seq_num, raw_data);
}


std::vector<std::string> ClientSender::generatePacketsData(char *raw_data, int data_size) {
  std::vector<std::string> result;
  int full_packets_qty = data_size / packet_size;
  int last_packet_size = data_size % packet_size;

  int total_packets_qty = full_packets_qty + (last_packet_size > 0 ? 1 : 0);
  for (int packet_idx = 0; packet_idx < total_packets_qty; packet_idx++) {
    int current_packet_sz;
    if (packet_idx == full_packets_qty) { // last non full packet
      current_packet_sz = last_packet_size;
    } else {
      current_packet_sz = packet_size;
    }

    auto packet_raw_str = std::string(raw_data + packet_idx * packet_size,
                                      raw_data + packet_idx * packet_size + current_packet_sz);

    result.emplace_back(packet_raw_str);
  }
  return result;
}


void ClientSender::sendPacketReliable(const std::string &raw_data) {
  {
    std::unique_lock<std::mutex> lk(packet_send_mutex);
    std::cout << "<>Waiting.." << std::endl;
    packet_send_cv.wait(lk, [cs = this]() {
      auto next_seq_num_ = cs->getNextSeqNum();
      auto base_idx_ = cs->getBaseIdx();
      auto wsize_ = cs->getWindowSize();
      std::cout << "cs->getNextSeqNum() < cs->getBaseIdx() + cs->getWindowSize()" << std::endl <<
                next_seq_num_ << "  <  " << base_idx_ << " + " << wsize_ << " = " << base_idx_ + wsize_ << std::endl;

      std::lock_guard<std::mutex> lg(cs->ackcount_mutex);
      std::cout << "last acked packet: " << cs->last_acked_packet << std::endl;
      return ((next_seq_num_ == 0 && (cs->last_acked_packet == cs->max_seq_num_b10 || cs->last_acked_packet <= 0)) ||
              next_seq_num_ != 0)
             && next_seq_num_ < base_idx_ + wsize_;
    });
    std::cout << "<>OK" << std::endl;

  }

  auto next_seq_num = getNextSeqNum();
  std::cout << "next_seq_num: " << next_seq_num << std::endl;
  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    max_seq_num_sent = next_seq_num;
    if (next_seq_num == 0 && (last_acked_packet == max_seq_num_b10)) {
      last_acked_packet = -1;
    }
  }

  window_map[next_seq_num] = std::make_unique<Packet>(generatePacket(raw_data, next_seq_num));
  auto packet = *(window_map[next_seq_num]);

  std::cout << "sendPacketReliable, sending: '" << packet.generatePacketString() << "'" << std::endl;
  sendPacketUnreliable(packet.generatePacketString());

  std::cout << "getBaseIdx() == getNextSeqNum()?, base_idx: " << getBaseIdx() << ", nextseqnum: " << getNextSeqNum()
            << std::endl;

  if (getBaseIdx() == getNextSeqNum()) {
    startTimeoutTimer();
  }
  incNextSeqNum();
  packet_send_cv.notify_all();
}


void ClientSender::timeoutHandler() {
  std::cout << "timeout... timeoutHandler working" << std::endl;
  for (int i = getBaseIdx(); i < getNextSeqNum(); i++) {
    sendPacketUnreliable(window_map[i]->generatePacketString());
  }
  startTimeoutTimer();
}

bool ClientSender::sendChunk() {
  if (dataProcessor.remainingChunks() == 0) {
    return false;
  }
  auto *dataChunk = dataProcessor.readChunk();

  auto splittedData = generatePacketsData(dataChunk, dataProcessor.getCurrentChunkSize());
  for (auto &packet_data : splittedData) {
    sendPacketReliable(packet_data);
  }

  auto chunk_packets_qty = splittedData.size();

  std::unique_lock<std::mutex> ul(chunk_sent_mutex);
  chunk_sent_cv.wait(ul, [cs = this, chunk_packets_qty = chunk_packets_qty]() {
    auto current_acked = cs->getAckedPacketsCount();

    return current_acked == chunk_packets_qty;
  });

  cleanTimers();
  resetChunkVars();

}


void ClientSender::resetChunkVars() {
  setAckedPacketsCount(0);
}

void ClientSender::startTimeoutTimer() {

  std::lock_guard<std::mutex> lg(timer_mutex);
  if (timeoutTimer) {
    timeoutTimer->stop();
  }
  olderTimers.push_back(std::move(timeoutTimer));

  timeoutTimer = std::move(startTimeout<std::function<void()>>(TIMEOUT_MILLISECONDS_WAIT,
                                                               std::bind(&ClientSender::timeoutHandler, this)));
}


void ClientSender::cleanTimers() {
  std::lock_guard<std::mutex> lg(timer_mutex);
  while (!olderTimers.empty() &&
         (!olderTimers.front() || !olderTimers.front().get() || olderTimers.front()->is_finished())) {
    olderTimers.pop_front();
  }
}


void ClientSender::threadFun() {
  while (sendChunk()) {};
  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    finished_sending_data = true;
  }
  endConnection();
  {
    std::lock_guard<std::mutex> lg(timer_mutex);
    if (timeoutTimer && timeoutTimer->has_started()) {
      timeoutTimer->stop();
    }
  }
  clientSubject->stopReceiver();
  std::cout << "ending connection" << std::endl;
}

void ClientSender::ackedPacket(int seqnum) {
  std::cout << "Acking packet: " << seqnum << std::endl;
  cleanTimers();

  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    if (finished_sending_data) {
      endConnection();
      return;
    }
    if (seqnum > max_seq_num_sent) {
      std::cout << "seqnum > max_seq_num_sent:" << seqnum << " > " << max_seq_num_sent << std::endl;
      return;
    }
    //std::cout << "Acked packet, seqnum: " << seqnum << ", last_acked_packet =" << last_acked_packet << std::endl;

    if (seqnum < last_acked_packet) {
      std::cout << "seqnum < last_acked_packed: " << seqnum << " < " << last_acked_packet << std::endl;
      return;
    } else {
      //std::cout << "continuing" << std::endl;
    }
  }

  if (seqnum + 1 == getBaseIdx()) {
    return;
  }


  setBaseIdx(seqnum + 1);
  if (getBaseIdx() == getNextSeqNum()) {
    std::cout << "getBaseIdx() == getNextSeqNum()" << std::endl;
    std::lock_guard<std::mutex> lg(timer_mutex);
    timeoutTimer->stop();
  } else {
    std::cout << "startTimeoutTimer" << std::endl;
    startTimeoutTimer();
  }

  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    std::cout << "seqnum - last_acked_packet=" << seqnum << "-" << last_acked_packet << "="
              << seqnum - last_acked_packet << std::endl;
    acked_count += seqnum - last_acked_packet;
    last_acked_packet = seqnum;
  }


  packet_send_cv.notify_all();
  chunk_sent_cv.notify_all();
}


int ClientSender::getAckedPacketsCount() {
  std::lock_guard<std::mutex> lg(ackcount_mutex);
  auto result = acked_count;
  return result;
}


void ClientSender::setAckedPacketsCount(int newval) {
  std::lock_guard<std::mutex> lg(ackcount_mutex);
  acked_count = newval;

}


void ClientSender::sendPacketUnreliable(const std::string &data, int ms_to_sleep) {
  socket.send_to(asio::buffer(data), endpoint);
  if (ms_to_sleep > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_sleep));
  }
}

void ClientSender::endConnection() {
  std::cout << "Trying to disconnect" << std::endl;
  sendPacketUnreliable("", 0);
}

int ClientSender::getNextSeqNum() {
  std::lock_guard<std::mutex> lk(nqn_mutex);
  auto result = next_seq_num_idx;
  return result;
}

void ClientSender::incNextSeqNum() {
  std::lock_guard<std::mutex> lk(nqn_mutex);
  next_seq_num_idx = (next_seq_num_idx + 1) % (max_seq_num_b10 + 1);
}

int ClientSender::getBaseIdx() {
  std::lock_guard<std::mutex> lk(bidx_mutex);
  auto result = base_idx;
  return result;
}

void ClientSender::setBaseIdx(int new_value) {
  std::lock_guard<std::mutex> lk(bidx_mutex);
  base_idx = new_value % (max_seq_num_b10 + 1);
}


int ClientSender::getWindowSize() {
  return window_size;
}

