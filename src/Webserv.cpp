#include "Webserv.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include "Connection.hpp"
#include "Listener.hpp"
#include "Server.hpp"
#include "Webserv.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollFd.hpp"
#include "ip/IpAddress.hpp"

#define MAX_EVENTS 1024

/*
 * The `size` argument in epoll_create is just for backwards compatibility.
 * Doesn't do anything since Linux kernel v2.6.8, only needs to be greater
 * than zero.
 */
Webserv::Webserv(std::string config_file) : epoll_fd_(epoll_create(1024))
{
  (void)config_file;
  if (epoll_fd_ == -1)
  {
    throw std::runtime_error("Unable to create epoll fd");
  }
}

Webserv::~Webserv()
{
  typedef std::map< const int, EpollFd* >::iterator iter_type;

  for (iter_type it = fds_.begin(); it != fds_.end(); ++it)
  {
    delete it->second;
  }
  close(epoll_fd_);
}

void Webserv::addServer(const std::vector< IpAddress* >& listeners,
                        const Server& server)
{
  typedef std::vector< IpAddress* >::const_iterator iter_type;

  for (iter_type it = listeners.begin(); it < listeners.end(); ++it)
  {
    Listener& listener = getListener(*it);
    listener.addServer(server);
  }
}

Listener& Webserv::getListener(IpAddress* addr)
{
  Listener* listener;

  std::map< const IpAddress*, int >::iterator fd_it = listeners_.find(addr);

  if (fd_it != listeners_.end())
  {
    listener = static_cast< Listener* >(fds_[fd_it->second]);
  }
  else
  {
    listener = new Listener(addr);
    int fd = listener->getFd();
    listeners_[addr] = fd;
    fds_[fd] = listener;
  }

  return *listener;
}

void Webserv::addFd(int fd, struct epoll_event* event)
{
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, event) == -1)
  {
    throw std::runtime_error("Unable to add fd to epoll");
  }

  fds_[fd] = static_cast< EpollFd* >(event->data.ptr);
}

void Webserv::modifyFd(int fd, struct epoll_event* event) const
{
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, event) == -1)
  {
    throw std::runtime_error("Unable to modify epoll event");
  }
}

void Webserv::deleteFd(int fd)
{
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1)
  {
    throw std::runtime_error("Unable to remove fd from epoll");
  }
  close(fd);

  delete fds_[fd];
  fds_.erase(fd);
}

void Webserv::addFdsToEpoll() const
{
  typedef std::map< int, EpollFd* >::const_iterator iter_type;

  for (iter_type it = fds_.begin(); it != fds_.end(); ++it)
  {
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, it->first, it->second->getEvent());
  }
}

void Webserv::mainLoop()
{
  extern volatile sig_atomic_t g_signal;
  struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

  addFdsToEpoll();

  while (true)
  {
    int count = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);

    if (g_signal || count == -1)
    {
      std::cerr << "\nSignal received, shutdown server\n";
      break;
    }

    for (int j = 0; j < count; ++j)
    {
      EpollFd* fd = static_cast< EpollFd* >(events[j].data.ptr);
      if (events[j].events & EPOLLRDHUP)
      {
        deleteFd(fd->getFd());
        continue;
      }

      EpollAction action = fd->epollCallback(events[j].events);

      switch (action.op)
      {
        case EPOLL_ACTION_ADD:
          addFd(action.fd, action.event);
          break;
        case EPOLL_ACTION_MOD:
          modifyFd(action.fd, action.event);
          break;
        case EPOLL_ACTION_DEL:
          deleteFd(action.fd);
          break;
        default:;  // Do nothing on EPOLL_ACTION_UNCHANGED
      }
    }

    pingAllClients();
  }

  delete[] events;
}

/*
 * The reason why this is pushing onto a vector and then deleting it in a
 * separate function is that deleting here immediately would fuck up the
 * iterator when deleting the fd from the map.
 */
void Webserv::pingAllClients()
{
  EpollMap::iterator it;
  std::vector< int > close_fds;

  for (it = fds_.begin(); it != fds_.end(); ++it)
  {
    Connection* c = dynamic_cast< Connection* >(it->second);
    if (c)
    {
      EpollAction action = c->ping();
      switch (action.op)
      {
        case EPOLL_ACTION_DEL:
          close_fds.push_back(action.fd);
          break;
        case EPOLL_ACTION_MOD:
          modifyFd(action.fd, action.event);
          break;
        default:;
      }
    }
  }

  closeClientConnections(close_fds);
}

void Webserv::closeClientConnections(const std::vector< int >& fds)
{
  std::vector< int >::const_iterator it;

  for (it = fds.begin(); it != fds.end(); ++it)
  {
    deleteFd(*it);
  }
}
