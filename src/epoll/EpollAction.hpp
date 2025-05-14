#pragma once

#include <sys/epoll.h>

enum EpollActions
{
  EPOLL_ACTION_ADD = EPOLL_CTL_ADD,
  EPOLL_ACTION_DEL = EPOLL_CTL_DEL,
  EPOLL_ACTION_MOD = EPOLL_CTL_MOD,
  EPOLL_ACTION_UNCHANGED
};

struct EpollAction
{
  int fd;
  EpollActions op;
  struct epoll_event* event;
};
