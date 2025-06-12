#include "EpollFd.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>

EpollFd::EpollFd() : fd_(-1)
{
  ep_event_ = new struct epoll_event();
  ep_event_->events = EPOLLIN | EPOLLRDHUP;
  ep_event_->data.ptr = this;
}

EpollFd::EpollFd(const EpollFd& other)
{
  if (other.fd_ == -1)
  {
    fd_ = -1;
  }
  else
  {
    fd_ = dup(other.fd_);
    if (fd_ == -1)
    {
      throw std::runtime_error("Unable to duplicate fd");
    }
  }
  ep_event_ = new epoll_event();
  ep_event_->events = other.ep_event_->events;
  ep_event_->data.ptr = this;
}

// Dummy because private
EpollFd& EpollFd::operator=(const EpollFd& other)
{
  (void)other;
  return *this;
}

EpollFd::~EpollFd()
{
  if (fd_ != -1)
  {
    close(fd_);
  }
  delete ep_event_;
}

int EpollFd::getFd() const
{
  return fd_;
}

struct epoll_event* EpollFd::getEvent() const
{
  return ep_event_;
}
