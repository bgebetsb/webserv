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

Webserv::Webserv() {
  /*
   * The `size` argument in epoll_create is just for backwards compatibility.
   * Doesn't do anything since Linux kernel v2.6.8, only needs to be greater
   * than zero.
   */
  epoll_fd_ = epoll_create(1024);
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

void Webserv::startListeners() {
  typedef std::vector< Listener >::iterator iter_type;
  int fd;
  extern volatile sig_atomic_t g_signal;
  struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

  for (iter_type it = listeners_.begin(); it < listeners_.end(); ++it) {
    fd = it->listen();
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, it->getEpollEvent()) < 0) {
      throw std::runtime_error("Unable to add fd to epoll fd");
    }
  }

  while (true) {
    int count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

    if (g_signal || count == -1) {
      std::cerr << "Signal received, shutdown server\n";
      break;
    }

    for (int j = 0; j < count; ++j) {
      EpollEventData* data = static_cast< EpollEventData* >(events[j].data.ptr);
      if (data->getType() == LISTENING_SOCKET) {}
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
