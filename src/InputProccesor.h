#ifndef PROJECT_INPUTPROCCESOR_H
#define PROJECT_INPUTPROCCESOR_H

#include <exception>
#include <string>
#include <memory>

#include <sstream>

#include "Client.h"

class InvalidClientInput : public std::exception {
  std::string output_msg;
public:
  InvalidClientInput(const std::string &option, const std::string &description, const std::string &reason) {
    std::stringstream ss_err;
    ss_err
            << "Invalid Client Input\n"
            << "\tOption '" << option << "'\n"
            << "\tDescription: '" << description << "'\n"
            << "\tReason: '" << reason << "'\n";
    output_msg = ss_err.str();
  }

  InvalidClientInput(const std::string &reason) {
    output_msg = reason;
  }

  char const *what() const noexcept {
    return output_msg.c_str();
  }
};

std::unique_ptr<Client> initClientFromInput(int argc, char **argv);


#endif //PROJECT_INPUTPROCCESOR_H
