#include "Webserv.hpp"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include "Listener.hpp"
#include "Server.hpp"
#include "Webserv.hpp"
#include "epoll/EpollEventData.hpp"

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
  close(epoll_fd_);
}

void Webserv::addServer(const std::vector< Listener >& listeners,
                        const Server& server) {
  typedef std::vector< Listener >::const_iterator iter_type;

  for (iter_type it = listeners.begin(); it < listeners.end(); ++it) {
    std::vector< Listener >::iterator listener =
        std::find(listeners_.begin(), listeners_.end(), *it);
    if (listener == listeners_.end()) {
      listeners_.push_back(*it);
      listener = listeners_.end() - 1;
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
  typedef std::vector< Listener >::iterator iter_type;
  extern volatile sig_atomic_t g_signal;
  struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

  for (iter_type it = listeners_.begin(); it < listeners_.end(); ++it) {
    it->listen();
  }

  while (true) {
    int count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

    if (g_signal || count == -1) {
      std::cerr << "\nSignal received, shutdown server\n";
      break;
    }

    for (int j = 0; j < count; ++j) {
      EpollFd* fd = static_cast< EpollFd* >(events[j].data.ptr);
      fd->epollCallback(events[j].events);
      // data->callback(events[j].events);
      // if (data->getType() == LISTENING_SOCKET) {
      //   std::cout << data << "\n";
      //   for (iter_type it = listeners_.begin(); it < listeners_.end(); ++it)
      //   {
      //     if (it->getFd() == data->getFd()) {
      //       it->acceptConnection(events[j].events, data);
      //     }
      //   }
      // }
    }
    //   if (events[j].data.fd == fd) {
    //     std::cout << "Trying to accept new fd\n";
    //     struct sockaddr_in peer_addr;
    //     socklen_t peer_addr_size = sizeof(peer_addr);
    //     int cfd = accept(fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
    //     if (cfd == -1) {
    //       std::cerr << "Accept failure\n";
    //       break;
    //     }
    //     std::cout << "Success\n";
    //     struct epoll_event* ep_event = new struct epoll_event();
    //     ep_event->data.fd = cfd;
    //     epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, cfd, ep_event);
    //     continue;
    //   }
    //   // std::cout << "FD: " << events[j].data.fd << "\n";
    //   // std::cout << "POLLIN: " << (events[j].events & EPOLLIN) << "\n";
    //   // std::cout << "POLLOUT: " << (events[j].events & EPOLLOUT) << "\n";
    //   // if (events[j].events & EPOLLRDHUP) {
    //   //   std::cout << "FD disconnected: " << events[j].data.fd << "\n";
    //   //   epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[j].data.fd, NULL);
    //   // }
    //   // std::cout << "EPOLLRDHUP: " << (events[j].events & EPOLLRDHUP) <<
    //   // "\n";
    // }
  }
}
