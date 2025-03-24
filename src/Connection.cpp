#include "Connection.hpp"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <set>
#include <stdexcept>
#include "epoll/EpollConnectionEventData.hpp"
#include "epoll/EpollEventData.hpp"

Connection::Connection(int socket_fd,
                       int epoll_fd,
                       const std::set< Server >& servers)
    : servers_(servers) {
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  fd_ = accept(socket_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
  if (fd_ == -1) {
    throw std::runtime_error("Unable to accept client connection");
  }

  ep_event_.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;

  EpollEventData* data = new EpollConnectionEventData(fd_, *this);
  ep_event_.data.ptr = data;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_, &ep_event_) < 0) {
    close(fd_);
    throw std::runtime_error("Unable to add fd to epoll fd");
  }
}

Connection::~Connection() {
  close(fd_);
}

void Connection::handleConnection(int type) {
  std::cout << "Connection callback received of type " << type << "\n";
}
