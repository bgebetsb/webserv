#pragma once

#include "../Connection.hpp"

enum EpollFdTypes {
  LISTENING_SOCKET,
  CLIENT_CONNECTION,
  LOCAL_FILE,
  CGI_IO
};

class EpollEventData {
 public:
  EpollEventData(int fd, EpollFdTypes type, Connection* connection);
  ~EpollEventData();
  EpollFdTypes getType() const;

 private:
  int fd_;
  EpollFdTypes fd_type_;
  Connection* connection_;
};
