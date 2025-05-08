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

  struct epoll_event* getEpollEvent();
  bool operator==(const Listener& other) const;
  EpollAction epollCallback(int event);

 private:
  void setup();
  EpollAction acceptConnection();
  Listener(const Listener& other);
  Listener& operator=(const Listener& other);

  const IpAddress* address_;
  std::set< Server > servers_;
};
