#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <map>
#include "epoll/EpollFd.hpp"

typedef std::map< const int, EpollFd* > EpollMap;

struct EpollData
{
  int fd;
  EpollMap fds;

  EpollData();
  ~EpollData();
};

EpollData& getEpollData();
