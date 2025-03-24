#pragma once

enum EpollFdTypes {
  LISTENING_SOCKET,
  CLIENT_CONNECTION,
  CGI_IO
};

class EpollEventData {
 public:
  // EpollEventData(int fd, EpollFdTypes type, Connection* connection);
  virtual ~EpollEventData();
  EpollFdTypes getType() const;
  int getFd() const;
  virtual void callback(int event) const = 0;

 protected:
  int fd_;
  EpollFdTypes fd_type_;
};
