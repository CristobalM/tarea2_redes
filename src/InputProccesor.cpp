#include "InputProccesor.h"
#include "cxxopts/cxxopts.hpp"


const char *SERVER_HOST = "server_host";
const char *FILE_NAME = "file_name";
const char *WINDOW_SIZE = "window_size";
const char *PACKET_SIZE = "packet_size";
const char *MAX_SEQ_NUMBER = "max_sequence_number";
const char *SERVER_PORT = "server_port";
const char *CLIENT_PORT = "client_port";


std::unique_ptr<Client> initClientFromInput(int argc, char **argv) {
  cxxopts::Options options("SecureOverUDPClient", "Secure Over UDP Client");

  struct SummaryOptions {
    std::string option, description;
    std::shared_ptr<cxxopts::Value> value;
  };

  std::vector<SummaryOptions> opts = {
          {SERVER_HOST,    "Server host ip address",        cxxopts::value<std::string>()},
          {FILE_NAME,      "File name to send",             cxxopts::value<std::string>()},
          {WINDOW_SIZE,    "Window size in packets number", cxxopts::value<int>()},
          {PACKET_SIZE,    "Size of each packet",           cxxopts::value<int>()},
          {MAX_SEQ_NUMBER, "Size of each packet",           cxxopts::value<int>()},
          {SERVER_PORT,    "Server port",                   cxxopts::value<int>()},
          {CLIENT_PORT,    "Client port",                   cxxopts::value<int>()}
  };

  auto acc_opts = options.add_options();
  for (auto &opt : opts) {
    acc_opts(opt.option, opt.description, opt.value);
  }

  std::unique_ptr<cxxopts::ParseResult> result_opt_ptr;

  try {
    result_opt_ptr = std::make_unique<cxxopts::ParseResult>(options.parse(argc, argv));
  }
  catch (const cxxopts::argument_incorrect_type &e) {
    throw InvalidClientInput("Invalid Input: Type Error\nReason: " + std::string(e.what()));
  }

  auto result_opt = *result_opt_ptr;

  for (auto &opt : opts) {
    if (result_opt[opt.option].count() < 1) {
      throw InvalidClientInput(opt.option, opt.description, "Option is missing");
    }
  }

  auto client = std::make_unique<Client>(
          result_opt[SERVER_HOST].as<std::string>(),
          result_opt[FILE_NAME].as<std::string>(),
          result_opt[WINDOW_SIZE].as<int>(),
          result_opt[PACKET_SIZE].as<int>(),
          result_opt[MAX_SEQ_NUMBER].as<int>(),
          result_opt[SERVER_PORT].as<int>(),
          result_opt[CLIENT_PORT].as<int>()
  );

  return client;
}
