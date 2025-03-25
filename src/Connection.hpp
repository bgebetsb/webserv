#pragma once

#include <sys/epoll.h>
#include <set>
#include "Server.hpp"
#include "epoll/EpollFd.hpp"

class Connection : public EpollFd {
 public:
  Connection(Webserv& webserver,
             int socket_fd,
             const std::set< Server >& servers);
  ~Connection();
  void epollCallback(int event);

 private:
  const std::set< Server >& servers_;
  struct epoll_event ep_event_;
  char* readbuf_;
  std::string buffer_;

  Connection(const Connection& other);
  Connection& operator=(const Connection& other);
  void handleRead(int type);
  void handleWrite(int type);
};
