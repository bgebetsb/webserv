#pragma once

#include <sys/epoll.h>
#include <set>
#include "Server.hpp"

class Connection {
 public:
  Connection(int socket_fd, int epoll_fd, const std::set< Server >& servers);
  ~Connection();
  void handleConnection(int type);

 private:
  int fd_;
  const std::set< Server >& servers_;
  struct epoll_event ep_event_;

  Connection(const Connection& other);
  Connection& operator=(const Connection& other);
};
