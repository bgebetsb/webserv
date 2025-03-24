#include "Connection.hpp"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include "Webserv.hpp"
#include "epoll/EpollEventData.hpp"

Connection::Connection(Webserv& webserver,
                       int socket_fd,
                       const std::set< Server >& servers)
    : EpollFd(webserver), servers_(servers) {
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  fd_ = accept(socket_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
  if (fd_ == -1) {
    throw std::runtime_error("Unable to accept client connection");
  }

  ep_event_.events = EPOLLIN | EPOLLRDHUP;

  ep_event_.data.ptr = this;
  webserver_.addFd(fd_, &ep_event_);

  // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_, &ep_event_) < 0) {
  //   close(fd_);
  //   throw std::runtime_error("Unable to add fd to epoll fd");
  // }
}

Connection::~Connection() {}

void Connection::epollCallback(int event) {
  handleConnection(event);
}

void Connection::handleConnection(int type) {
  std::cout << "Connection callback received of type " << type << "\n";
  char* buf = new char[1024];
  ssize_t ret = recv(fd_, buf, 1024, 0);
  if (ret == -1) {
    throw std::runtime_error("Recv failed");
  }
  buffer_.append(buf, ret);
  size_t pos = buffer_.find("\n");
  if (pos != std::string::npos) {
    std::cout << buffer_;
    buffer_.clear();
  }
}
