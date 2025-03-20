#include "Listener.hpp"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include "epoll/EpollEventData.hpp"

Listener::Listener(u_int32_t ip, u_int16_t port)
    : ip_(ip), port_(port), socket_fd_(-1), epoll_fd_(-1) {}

Listener::~Listener() {
  if (socket_fd_ != -1) {
    close(socket_fd_);
  }
}

int Listener::listen() {
  if (epoll_fd_ == -1) {
    throw std::runtime_error("Called listen() without setting epoll fd first");
  }

  setup();
  if (::listen(socket_fd_, 0) == -1) {
    throw std::runtime_error("Unable to listen");
  }

  return socket_fd_;
}

void Listener::setup() {
  struct sockaddr_in addr;

  socket_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (socket_fd_ == -1) {
    throw std::runtime_error("Unable to create socket");
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip_;
  addr.sin_port = port_;

  int reuse = 1;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) ==
      -1) {
    close(socket_fd_);
    throw std::runtime_error("Unable to set socket options");
  }

  if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    close(socket_fd_);
    throw std::runtime_error("Unable to bind socket to address");
  }

  ep_event_.events = EPOLLIN | EPOLLRDHUP;
  EpollEventData* data = new EpollEventData(socket_fd_, LISTENING_SOCKET, NULL);

  ep_event_.data.ptr = data;
}

struct epoll_event* Listener::getEpollEvent() {
  return &ep_event_;
}

bool Listener::operator<(const Listener& other) const {
  return (ip_ < other.ip_ || (ip_ == other.ip_ && port_ < other.port_));
}

bool Listener::operator==(const Listener& other) const {
  return (ip_ == other.ip_ && port_ == other.port_);
}

void Listener::addServer(const Server& server) {
  if (!servers_.insert(server).second) {
    std::cerr << "Server not added due to ip/port/server_name duplicate\n";
  }
}

void Listener::setEpollfd(int fd) {
  epoll_fd_ = fd;
}
