#include "Webserv.hpp"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include <stdexcept>
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
Webserv::Webserv() : epoll_fd_(epoll_create(1024)) {
  if (epoll_fd_ == -1) {
    throw std::runtime_error("Unable to create epoll fd");
  }
}

Webserv::~Webserv() {
  typedef std::map< const int, EpollFd* >::iterator iter_type;

  std::cout << "Size of map: " << fds_.size() << "\n";
  for (iter_type it = fds_.begin(); it != fds_.end(); ++it) {
    std::cout << "Deleting sth\n";
    delete it->second;
  }
  close(epoll_fd_);
}

void Webserv::addServer(const std::vector< IpAddress* >& listeners,
                        const Server& server) {
  typedef std::vector< IpAddress* >::const_iterator iter_type;

  for (iter_type it = listeners.begin(); it < listeners.end(); ++it) {
    Listener* listener;
    try {
      int fd = listeners_.at(*it);
      listener = static_cast< Listener* >(fds_[fd]);
    } catch (std::out_of_range& e) {
      listener = new Listener(*it);
      int fd = listener->getFd();
      listeners_[*it] = fd;
      fds_[fd] = listener;
    }

    listener->addServer(server);
  }
}

void Webserv::addFd(int fd, struct epoll_event* event) {
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, event) == -1) {
    throw std::runtime_error("Unable to add fd to epoll");
  }

  fds_[fd] = static_cast< EpollFd* >(event->data.ptr);
}

void Webserv::modifyFd(int fd, struct epoll_event* event) const {
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, event) == -1) {
    throw std::runtime_error("Unable to modify epoll event");
  }
}

void Webserv::deleteFd(int fd) {
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
    throw std::runtime_error("Unable to remove fd from epoll");
  }

  // TODO: Probably I should also delete the data.ptr inside the event here
  delete fds_[fd];
  fds_.erase(fd);
}

void Webserv::startListeners() {
  typedef std::map< int, EpollFd* >::iterator iter_type;
  extern volatile sig_atomic_t g_signal;
  struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

  std::cout << "Map has size " << fds_.size() << "\n";
  for (iter_type it = fds_.begin(); it != fds_.end(); ++it) {
    std::cout << "Adding epoll for fd " << it->first << "\n";
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, it->first, it->second->getEvent());
  }
  // for (iter_type it = fds_.begin(); it < fds_.end(); ++it) {
  // EpollAction action = it->listen();
  // fds_[action.fd] = static_cast< EpollFd* >(action.event->data.ptr);
  // epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, action.fd, action.event);
  // }

  while (true) {
    int count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

    if (g_signal || count == -1) {
      std::cerr << "\nSignal received, shutdown server\n";
      break;
    }

    for (int j = 0; j < count; ++j) {
      EpollFd* fd = static_cast< EpollFd* >(events[j].data.ptr);
      if (events[j].events & EPOLLRDHUP) {
        fds_.erase(fd->getFd());
        delete fd;
        continue;
      }
      EpollAction action = fd->epollCallback(events[j].events);
      if (action.op != EPOLL_ACTION_UNCHANGED) {
        epoll_ctl(epoll_fd_, action.op, action.fd, action.event);
        if (action.op == EPOLL_ACTION_ADD) {
          fds_[action.fd] = static_cast< EpollFd* >(action.event->data.ptr);
        } else if (action.op == EPOLL_ACTION_DEL) {
          fds_.erase(action.fd);
          delete fd;
        }
      }
    }
  }
  delete[] events;
}
