#include <iostream>

#include <fstream>

#include "Client.h"
#include "Timer.h"

const int CHUNK_SIZE = 1 << 18;//256;//1 << 16;

Client::Client(const std::string &server_host,
               const std::string &file_name,
               int window_size,
               int packet_size,
               int max_seq_number,
               int server_port,
               int client_port) :
        server_host(server_host),
        file_name(file_name),
        window_size(window_size),
        packet_size(packet_size),
        max_seq_number(max_seq_number),
        server_port(server_port),
        client_port(client_port),
        clientSender(CHUNK_SIZE, this),
        clientReceiver(this) {
}

void Client::stopReceiver() {
  clientReceiver.stop();
}

const std::string &Client::getServer_host() const {
  return server_host;
}

const std::string &Client::getFile_name() const {
  return file_name;
}

int Client::getWindow_size() const {
  return window_size;
}

int Client::getPacket_size() const {
  return packet_size;
}

int Client::getMax_seq_number() const {
  return max_seq_number;
}

int Client::getServer_port() const {
  return server_port;
}

int Client::getClient_port() const {
  return client_port;
}


int Client::start() {

  clientReceiver.start();
  clientSender.start();


  return 0;
}

void Client::ackedPacket(int seqnum) {
  clientSender.ackedPacket(seqnum);
}

std::string Client::getFilename() const {
  return file_name;
}

