#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
#include "../Configs/Configs.hpp"
#include "../ip/IpAddress.hpp"
#include "EpollAction.hpp"
#include "EpollFd.hpp"

class Listener : public EpollFd
{
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
  std::vector< Server > servers_;
};
