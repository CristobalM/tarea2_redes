#ifndef PROJECT_CLIENTRECEIVER_H
#define PROJECT_CLIENTRECEIVER_H

#include <thread>
#include "asio.hpp"

#include "ClientSubject.h"

using asio::ip::udp;

class ClientReceiver {
  ClientSubject *clientSubject;

  int window_size;
  int packet_size;
  int client_port;
  int max_seq_number;

  int max_seq_num_b10;

  std::unique_ptr<std::thread> receiver_thread;

  void threadFun();


  asio::io_service io_service;
  udp::socket socket;

public:
  explicit ClientReceiver(ClientSubject *clientSubject);

  ~ClientReceiver();

  void start();

  void stop();

};


#endif //PROJECT_CLIENTRECEIVER_H
