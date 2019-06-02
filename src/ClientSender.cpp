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
        finished_sending_data(false)
        {
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


std::unique_ptr<Packet> ClientSender::generatePacket(const std::string &raw_data, int packet_seq_num) {
  return std::make_unique<Packet>(max_seq_number, packet_seq_num, raw_data);
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



bool ClientSender::canAdvance() {
  auto next_seq_num_ = this->getNextSeqNum();
  auto base_idx_ = this->getBaseIdx();
  auto wsize_ = this->getWindowSize();
  /*std::cout << "cs->getNextSeqNum() < cs->getBaseIdx() + cs->getWindowSize()" << std::endl <<
            next_seq_num_ << "  <  " << base_idx_ << " + " << wsize_ << " = " << base_idx_ + wsize_ << std::endl;*/

  std::lock_guard<std::mutex> lg(this->ackcount_mutex);
  //std::cout << "last acked packet: " << this->last_acked_packet << std::endl;

  return (base_idx_ <= next_seq_num_ && next_seq_num_ < base_idx_ + wsize_) ||
      (base_idx_ > next_seq_num_ && next_seq_num_ < (base_idx_ + wsize_ ) % (this->max_seq_num_b10 +1) );

  /*
  return ((next_seq_num_ == 0 && (cs->last_acked_packet == cs->max_seq_num_b10 || cs->last_acked_packet <= 0)) ||
          next_seq_num_ != 0)
         && next_seq_num_ < base_idx_ + wsize_;
         */
}


void ClientSender::sendPacketReliable(const std::string &raw_data) {

  std::unique_lock<std::mutex> lk(packet_send_mutex);
  //std::cout << "<>Waiting.." << std::endl;
  packet_send_cv.wait(lk, std::bind(&ClientSender::canAdvance, this));
  //std::cout << "<>OK" << std::endl;


  auto next_seq_num = getNextSeqNum();
  std::cout << "First time sending " << next_seq_num << std::endl;
  //std::cout << "next_seq_num: " << next_seq_num << std::endl;
  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    max_seq_num_sent = next_seq_num;
  }

  window_map[next_seq_num] = generatePacket(raw_data, next_seq_num);
  auto &packet = *(window_map[next_seq_num]);

  /*std::cout << "sendPacketReliable, sending: '" << packet.generatePacketString() << "'" << std::endl;*/
  packet.startTimeoutCount();
  sendPacketUnreliable(packet.generatePacketString());

  /*std::cout << "getBaseIdx() == getNextSeqNum()?, base_idx: " << getBaseIdx() << ", nextseqnum: " << getNextSeqNum()
            << std::endl;*/

  if (getBaseIdx() == getNextSeqNum()) {
    startTimeoutTimer();
  }
  incNextSeqNum();
  packet_send_cv.notify_all();
}


void ClientSender::timeoutHandler() {
  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    if (finished_sending_data) {
      std::cout << "timeoutHandler: Cancelling.." << std::endl;
      return;
    }
  }
  //std::cout << "timeout... timeoutHandler working" << std::endl;
  std::unique_lock<std::mutex> lk(packet_send_mutex);
  auto nseqnum = getNextSeqNum();
  auto bidx = getBaseIdx();
  auto buffer_size = (max_seq_num_b10 + 1);
  auto wsize = getWindowSize();
  for (int i = bidx; (nseqnum >= bidx && i < nseqnum) || (nseqnum < bidx && buffer_size - (bidx - nseqnum) <= wsize &&  (i >= bidx  || i < nseqnum)); i = (i+1) % buffer_size) {
    std::cout << "Resending packet " << i << std::endl;
    auto &packet = window_map[i];

    packet->setRetransmitted();
    sendPacketUnreliable(packet->generatePacketString());
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
    std::cout << "Current Acked: " << current_acked << "/"<< chunk_packets_qty << std::endl;

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

  std::lock_guard<std::mutex> rtt_lock(estimated_rtt_mutex);
  timeoutTimer = std::move(startTimeout<std::function<void()>>(estimated_rtt,
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
  std::cout << "last estimated rtt: " << estimated_rtt << std::endl;
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

    auto &packet = *window_map[seqnum];
    if(packet.getAcked()){
      return;
    }

    if(!packet.getRetransmitted()){
      std::lock_guard<std::mutex> rtt_lock(estimated_rtt_mutex);
      auto sampled_rtt = (std::chrono::duration_cast<std::chrono::milliseconds >(
          std::chrono::system_clock::now().time_since_epoch()) - packet.getStartingTime()).count();


      estimated_rtt = (int)((1 - ALPHA) * estimated_rtt + ALPHA * sampled_rtt);
      std::cout << "SAMPLED_RTT: " << sampled_rtt;
      std::cout << ". ESTIMATED_RTT: " << estimated_rtt << std::endl;
    }

  }

  setBaseIdx(seqnum + 1);
  auto current_base_idx = getBaseIdx();
  auto current_next_seqn = getNextSeqNum();
  std::cout << "Base IDx: " << current_base_idx << ", nextseqnum: " << current_next_seqn << std::endl;

  if (current_base_idx == current_next_seqn) {
    std::lock_guard<std::mutex> lg(timer_mutex);
    timeoutTimer->stop();
    std::cout << "current_base_idx == getNextSeqNum()";
    std::cout << "STOPPING TIMEOUT" << std::endl;
  } else {
    startTimeoutTimer();
  }

  {
    std::lock_guard<std::mutex> lg(ackcount_mutex);
    /*
    std::cout << "seqnum - last_acked_packet=" << seqnum << "-" << last_acked_packet << "="
              << seqnum - last_acked_packet << std::endl;*/

    if(seqnum < last_acked_packet){
      acked_count += (max_seq_num_b10 - last_acked_packet) + seqnum + 1;
    }
    else{
      acked_count += seqnum - last_acked_packet;
    }

    last_acked_packet = seqnum;
    auto &packet = *window_map[seqnum];
    packet.setAcked();
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
  //std::cout << "Sending: '" << data << "'" << std::endl;
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
  //std::cout << "UPDATING setBaseIdx" << std::endl;
  std::lock_guard<std::mutex> lk(bidx_mutex);
  base_idx = new_value % (max_seq_num_b10 + 1);
}


int ClientSender::getWindowSize() {
  return window_size;
}

void ClientSender::calculateEstimatedRTT() {


}
