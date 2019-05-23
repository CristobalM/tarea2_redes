#ifndef PROJECT_CLIENTSENDER_H
#define PROJECT_CLIENTSENDER_H


#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <functional>
#include <deque>

#include "asio.hpp"

#include "DataProcessor.h"
#include "ClientSubject.h"
#include "Packet.h"
#include "Timer.h"

using asio::ip::udp;

const int TIMEOUT_MILLISECONDS_WAIT = 1000;
const int UNRELIABLE_PACKET_DELAY_MS = 0;


using timer_uptr = std::unique_ptr<Timer<std::function<void()>>>;

class ClientSender {
  DataProcessor dataProcessor;
  ClientSubject *clientSubject;

  volatile int base_idx;
  volatile int next_seq_num_idx;
  volatile int acked_count;


  int window_size;
  int packet_size;
  std::string server_port;
  std::string server_host;

  int max_seq_number;
  int max_seq_num_b10;


  int last_acked_packet;

  int max_seq_num_sent;


  bool finished_sending_data = false;


  std::unique_ptr<std::thread> sender_thread;

  std::mutex packet_send_mutex;
  std::condition_variable packet_send_cv;

  std::mutex chunk_sent_mutex;
  std::condition_variable chunk_sent_cv;

  asio::io_service io_service;
  asio::io_context io_context;
  udp::socket socket;

  udp::resolver resolver;
  udp::resolver::query query;
  udp::resolver::iterator qiterator;
  udp::endpoint endpoint;


  std::vector<std::unique_ptr<Packet>> window_map;

  std::mutex bidx_mutex, nqn_mutex, ackcount_mutex, timer_mutex;


  void threadFun();

  bool sendChunk();

  Packet generatePacket(const std::string &raw_data, int packet_seq_num);

  std::vector<std::string> generatePacketsData(char *raw_data, int data_size);

  void sendPacketReliable(const std::string &raw_data);

  int getAckedPacketsCount();

  void setAckedPacketsCount(int newval);

  void timeoutHandler();

  timer_uptr timeoutTimer;

  std::deque<timer_uptr> olderTimers;

  void startTimeoutTimer();

  void sendPacketUnreliable(const std::string &data, int ms_to_sleep = UNRELIABLE_PACKET_DELAY_MS);

  void endConnection();

  void resetChunkVars();

  void cleanTimers();

public:
  ClientSender(int chunkSize, ClientSubject *clientSubject);

  ~ClientSender();

  void start();

  void ackedPacket(int seqnum);

  void incNextSeqNum();

  void setBaseIdx(int new_value);

  int getNextSeqNum();

  int getBaseIdx();

  int getWindowSize();


};


#endif //PROJECT_CLIENTSENDER_H
