#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <set>
#include <vector>
#include "Server.hpp"
#include "epoll/EpollEventData.hpp"
#include "ip/IpAddress.hpp"

class Listener {
 public:
  Listener(IpAddress* host);
  ~Listener();

  void listen();
  void addServer(const Server& server);
  void setEpollfd(int fd);

  struct epoll_event* getEpollEvent();
  // bool operator<(const Listener& other) const;
  bool operator==(const Listener& other) const;
  void acceptConnection(int event, EpollEventData* data);

 private:
  void setup();

  const IpAddress* address_;
  int socket_fd_;
  int epoll_fd_;
  struct epoll_event ep_event_;
  std::set< Server > servers_;
  std::vector< Connection > connections_;
};

void acceptConnection(Listener& listener, int event, EpollEventData* data);
