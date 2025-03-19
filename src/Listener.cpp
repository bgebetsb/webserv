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
    : ip_(ip), port_(port), fd_(-1) {}

Listener::~Listener() {
  if (fd_ != -1) {
    close(fd_);
  }
}

int Listener::listen() {
  setup();
  if (::listen(fd_, 0) == -1) {
    throw std::runtime_error("Unable to listen");
  }

  return fd_;
}

void Listener::setup() {
  struct sockaddr_in addr;

  fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd_ == -1) {
    throw std::runtime_error("Unable to create socket");
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip_;
  addr.sin_port = port_;

  int reuse = 1;
  if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    close(fd_);
    throw std::runtime_error("Unable to set socket options");
  }

  if (bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    close(fd_);
    throw std::runtime_error("Unable to bind socket to address");
  }

  ep_event.events = EPOLLIN | EPOLLRDHUP;
  EpollEventData* data = new EpollEventData(fd_, LISTENING_SOCKET, NULL);

  ep_event.data.ptr = data;
}

struct epoll_event* Listener::getEpollEvent() {
  return &ep_event;
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
