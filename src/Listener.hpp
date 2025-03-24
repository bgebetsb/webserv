#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <set>
#include "Server.hpp"
#include "epoll/EpollFd.hpp"
#include "ip/IpAddress.hpp"

class Listener : public EpollFd {
 public:
  Listener(Webserv& webserver, IpAddress* host);
  ~Listener();

  void listen();
  void addServer(const Server& server);
  // void setEpollfd(int fd);

  struct epoll_event* getEpollEvent();
  // bool operator<(const Listener& other) const;
  bool operator==(const Listener& other) const;
  void epollCallback(int event);
  // int getFd() const;

 private:
  void setup();
  void acceptConnection(int event);

  const IpAddress* address_;
  struct epoll_event* ep_event_;
  std::set< Server > servers_;
  // std::vector< Connection* > connections_;
};
