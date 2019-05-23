#ifndef PROJECT_CLIENTSUBJECT_H
#define PROJECT_CLIENTSUBJECT_H

#include <string>


class ClientSubject {
protected:
  virtual ~ClientSubject() = default;

public:

  virtual void ackedPacket(int seqnum) = 0;

  virtual int getWindow_size() const = 0;

  virtual int getPacket_size() const = 0;

  virtual int getMax_seq_number() const = 0;

  virtual int getServer_port() const = 0;

  virtual const std::string &getServer_host() const = 0;

  virtual int getClient_port() const = 0;

  virtual std::string getFilename() const = 0;

  virtual void stopReceiver() = 0;
};

#endif //PROJECT_CLIENTSUBJECT_H
