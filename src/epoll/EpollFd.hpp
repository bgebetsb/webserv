#pragma once

#include <sys/epoll.h>
#include "EpollAction.hpp"
// class Webserv;

class EpollFd
{
 public:
  EpollFd();
  EpollFd(const EpollFd& other);
  virtual ~EpollFd();
  virtual EpollAction epollCallback(int event) = 0;
  int getFd() const;
  struct epoll_event* getEvent() const;

 protected:
  int fd_;
  struct epoll_event* ep_event_;

 private:
  EpollFd& operator=(const EpollFd& other);
};
