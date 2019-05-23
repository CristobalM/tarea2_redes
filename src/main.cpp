#include <iostream>
#include <memory>

#include "asio.hpp"

#include "Client.h"
#include "InputProccesor.h"


int main(int argc, char **argv) {
  std::unique_ptr<Client> client;
  try {
    client = initClientFromInput(argc, argv);
  }
  catch (const InvalidClientInput &e) {
    std::cout << e.what() << std::endl;
    return 1;
  }

  std::cout
          << "server_host: " << client->getServer_host() << "\n"
          << "file_name: " << client->getFile_name() << "\n"
          << "window_size: " << client->getWindow_size() << "\n"
          << "packet_size: " << client->getPacket_size() << "\n"
          << "max_sequence_number: " << client->getMax_seq_number() << "\n"
          << "server_port: " << client->getServer_port() << "\n"
          << "client_port: " << client->getClient_port() << std::endl;


  return client->start();
}