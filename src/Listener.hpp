#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <set>
#include "Server.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollFd.hpp"
#include "ip/IpAddress.hpp"

class Listener : public EpollFd {
 public:
  Listener(IpAddress* host);
  ~Listener();

  void addServer(const Server& server);
  // void setEpollfd(int fd);

  struct epoll_event* getEpollEvent();
  // bool operator<(const Listener& other) const;
  bool operator==(const Listener& other) const;
  EpollAction epollCallback(int event);
  // int getFd() const;

 private:
  void setup();
  EpollAction acceptConnection(int event);

  const IpAddress* address_;
  std::set< Server > servers_;
  // std::vector< Connection* > connections_;
};
