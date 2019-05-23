#ifndef CLIENTE_T2_REDES_CLIENT_H
#define CLIENTE_T2_REDES_CLIENT_H


#include <string>
#include <vector>
#include "ClientSubject.h"
#include "ClientSender.h"
#include "ClientReceiver.h"


class Client : public ClientSubject {
  std::string server_host;
  std::string file_name;
  int window_size;
  int packet_size;
  int max_seq_number;
  int server_port;
  int client_port;

  ClientSender clientSender;
  ClientReceiver clientReceiver;
public:
  Client(const std::string &server_host,
         const std::string &file_name,
         int window_size,
         int packet_size,
         int max_seq_number,
         int server_port,
         int client_port);


  int start();


  // Getters
  const std::string &getServer_host() const;

  const std::string &getFile_name() const;

  int getWindow_size() const override;

  int getPacket_size() const override;

  int getMax_seq_number() const override;

  int getServer_port() const override;

  int getClient_port() const override;
  // End Getters

  void ackedPacket(int seqnum) override;

  std::string getFilename() const override;

  void stopReceiver() override;

};


#endif //CLIENTE_T2_REDES_CLIENT_H
