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
  struct epoll_event* getEpollEvent();
  bool operator<(const Listener& other) const;
  bool operator==(const Listener& other) const;
  void addServer(const Server& server);

 private:
  void setup();

  u_int32_t ip_;
  u_int16_t port_;
  int fd_;
  struct epoll_event ep_event;
  std::set< Server > servers_;
};
