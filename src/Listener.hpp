#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <set>
#include "Server.hpp"

class Listener {
 public:
  Listener(u_int32_t ip, u_int16_t port);
  ~Listener();

  int listen();
  void addServer(const Server& server);
  void setEpollfd(int fd);

  struct epoll_event* getEpollEvent();
  bool operator<(const Listener& other) const;
  bool operator==(const Listener& other) const;

 private:
  void setup();

  u_int32_t ip_;
  u_int16_t port_;
  int socket_fd_;
  int epoll_fd_;
  struct epoll_event ep_event_;
  std::set< Server > servers_;
};
