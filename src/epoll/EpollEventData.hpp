#pragma once

enum EpollFdTypes {
  LISTENING_SOCKET,
  CLIENT_CONNECTION,
  LOCAL_FILE,
  CGI_IO
};

class EpollEventData {
 public:
  EpollEventData(int fd, EpollFdTypes type);
  ~EpollEventData();

 private:
  int fd_;
  EpollFdTypes fd_type_;
};
