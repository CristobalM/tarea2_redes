#ifndef PROJECT_PACKET_H
#define PROJECT_PACKET_H


#include <memory>
#include <sstream>
#include <algorithm>

#include <string>
#include <iostream>

#include "digestpp-master/digestpp.hpp"


class Packet {
  int seqnum_digits;
  int seqnum;
  std::string packet_data;

  std::string packetString;
  bool cached;

public:
  Packet(int seqnum_digits, int seqnum, const std::string &packet_data) :
          seqnum_digits(seqnum_digits),
          seqnum(seqnum),
          packet_data(packet_data),
          cached(false) {
  }

  static std::string getHashFromString(const std::string &inputString) {
    return digestpp::md5().absorb(inputString).hexdigest();
  }

  static int getNumberOfDigits(int number) {
    if (number == 0) {
      return 1;
    }
    int result = 0;
    while (number > 0) {
      number /= 10;
      result++;
    }
    return result;
  }

  static bool checkPacketIntegrity(const std::string &raw_full_packet, int seqnum_digits, bool checkACK = true) {
    auto endOfMD5Str = seqnum_digits + 32;
    auto md5String = raw_full_packet.substr(seqnum_digits, endOfMD5Str);

    std::string dataString, dataStringHashed;

    if (checkACK) {
      dataString = raw_full_packet.substr(0, seqnum_digits);
    } else {
      dataString = raw_full_packet.substr(endOfMD5Str, raw_full_packet.length());
    }
    dataStringHashed = getHashFromString(dataString);

    return md5String == dataStringHashed;
  }

  static int extract_seq_num(const std::string &raw_full_packet, int seqnum_digits) {
    auto str_val = raw_full_packet.substr(0, seqnum_digits);
    std::string real_str_val;
    std::istringstream iss(str_val);
    int number;
    iss >> number;
    return number;
  }


  int getSeqNum() {
    return seqnum;
  }

  std::string generatePacketString() {
    if (cached) {
      return packetString;
    }
    std::stringstream ss;
    char md5buf[32];

    int extra_zeros = seqnum_digits - getNumberOfDigits(seqnum);
    for (int i = 0; i < extra_zeros; i++) {
      ss << "0";
    }
    ss << std::to_string(seqnum);
    ss << getHashFromString(packet_data);
    ss << packet_data;

    packetString = ss.str();
    cached = true;

    return packetString;
  }
};


#endif //PROJECT_PACKET_H
