#include "Connection.hpp"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include "epoll/EpollAction.hpp"

Connection::Connection(int socket_fd, const std::set< Server >& servers)
    : servers_(servers), polling_write_(false) {
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  readbuf_ = new char[1024];
  fd_ = accept(socket_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
  if (fd_ == -1) {
    throw std::runtime_error("Unable to accept client connection");
  }

  ep_event_->events = EPOLLIN | EPOLLRDHUP;

  ep_event_->data.ptr = this;

  // if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_, &ep_event_) < 0) {
  //   close(fd_);
  //   throw std::runtime_error("Unable to add fd to epoll fd");
  // }
}

Connection::~Connection() {
  delete[] readbuf_;
}

EpollAction Connection::epollCallback(int event) {
  if (event & EPOLLIN) {
    return handleRead(event);
  } else if (event & EPOLLOUT) {
    return handleWrite(event);
  }

  EpollAction action = {fd_, EPOLL_ACTION_DEL, NULL};
  return action;
}

EpollAction Connection::handleRead(int type) {
  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, NULL};

  std::cout << "Connection callback received of type " << type << "\n";
  ssize_t ret = recv(fd_, readbuf_, 1024, 0);
  if (ret == -1) {
    throw std::runtime_error("Recv failed");
  }
  buffer_.append(readbuf_, ret);
  size_t pos = buffer_.find("\n");
  if (pos != std::string::npos) {
    std::cout << buffer_;
    buffer_.clear();
    // TODO: This should only be called if its currently not polling for write
    if (!polling_write_) {
      ep_event_->events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
      action.op = EPOLL_ACTION_MOD;
      action.event = ep_event_;
    }
  }

  return action;
}

EpollAction Connection::handleWrite(int type) {
  std::cout << "Connection callback received of type " << type << "\n";
  std::string response("Full string received oida\r\n");
  ssize_t ret = send(fd_, response.c_str(),
                     std::min(static_cast< size_t >(1024), response.size()), 0);
  if (ret == -1) {
    throw std::runtime_error("Send failed");
  }
  // TODO: Store leftover if not everything has been sent yet
  ep_event_->events = EPOLLIN | EPOLLRDHUP;
  EpollAction action = {fd_, EPOLL_ACTION_MOD, ep_event_};

  return action;
}
