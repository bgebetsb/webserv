#pragma once

#include <sys/epoll.h>
#include <set>
#include "Server.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollFd.hpp"

class Connection : public EpollFd {
 public:
  Connection(int socket_fd, const std::set< Server >& servers);
  ~Connection();
  EpollAction epollCallback(int event);

 private:
  const std::set< Server >& servers_;
  char* readbuf_;
  std::string buffer_;
  bool polling_write_;

  Connection(const Connection& other);
  Connection& operator=(const Connection& other);
  EpollAction handleRead(int type);
  EpollAction handleWrite(int type);
};
