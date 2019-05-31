#include <iostream>
#include <cmath>

#include "ClientReceiver.h"

#include "Packet.h"

ClientReceiver::ClientReceiver(ClientSubject *clientSubject) :
        clientSubject(clientSubject),
        window_size(clientSubject->getWindow_size()),
        packet_size(clientSubject->getPacket_size()),
        client_port(clientSubject->getClient_port()),
        max_seq_number(clientSubject->getMax_seq_number()),
        socket(io_service, udp::endpoint(udp::v4(), (unsigned short) client_port)),
        max_seq_num_b10((int) (std::pow(10, max_seq_number) - 1)) {

}


void ClientReceiver::start() {
  receiver_thread = std::make_unique<std::thread>(&ClientReceiver::threadFun, this);
}

ClientReceiver::~ClientReceiver() {
  if (receiver_thread && receiver_thread->joinable()) {

    std::cout << "Joining receiver" << std::endl;
    receiver_thread->join();

  }
}

void ClientReceiver::threadFun() {
  std::cout << "receiver" << std::endl;
  int buffer_sz = packet_size + 1 + 32 + max_seq_number;
  auto tmp_buffer = std::make_unique<char[]>(buffer_sz);
  for (;;) {
    udp::endpoint endpoint;
    std::fill(tmp_buffer.get(), tmp_buffer.get() + buffer_sz, 0);
    size_t recv_length = socket.receive_from(asio::buffer(tmp_buffer.get(), buffer_sz), endpoint);
    if (recv_length == 0) {
      break;
    }

    std::string input_raw_packet(tmp_buffer.get());
    if (Packet::checkPacketIntegrity(input_raw_packet, max_seq_number)) {
      auto packet_seq_num = Packet::extract_seq_num(input_raw_packet, max_seq_number);
      //std::cout << "Receiver calling acked packet" << std::endl;
      clientSubject->ackedPacket(packet_seq_num);
    }
  }
}

void ClientReceiver::stop() {
  std::cout << "ClientReceiver::stop()" << std::endl;
  io_service.stop();
  socket.cancel();
}

