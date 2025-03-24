#include "Listener.hpp"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <new>
#include <stdexcept>
#include "Connection.hpp"
#include "epoll/EpollEventData.hpp"
#include "epoll/EpollListenerEventData.hpp"
#include "ip/IpAddress.hpp"

Listener::Listener(IpAddress* address)
    : address_(address), socket_fd_(-1), epoll_fd_(-1) {}

Listener::~Listener() {
  if (socket_fd_ != -1) {
    close(socket_fd_);
  }
}

void Listener::listen() {
  if (epoll_fd_ == -1) {
    throw std::runtime_error("Called listen() without setting epoll fd first");
  }

  setup();
  if (::listen(socket_fd_, 0) == -1) {
    throw std::runtime_error("Unable to listen");
  }

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_fd_, getEpollEvent()) < 0) {
    throw std::runtime_error("Unable to add fd to epoll fd");
  }
}

void Listener::setup() {
  socket_fd_ = address_->createSocket();

  ep_event_ = new struct epoll_event();
  ep_event_->events = EPOLLIN | EPOLLRDHUP;
  EpollEventData* data = new EpollListenerEventData(socket_fd_, *this);
  std::cout << data << "\n";

  ep_event_->data.ptr = data;
}

struct epoll_event* Listener::getEpollEvent() {
  return ep_event_;
}

// bool Listener::operator<(const Listener& other) const {
//   return (ip_ < other.ip_ || (ip_ == other.ip_ && port_ < other.port_));
// }
//
bool Listener::operator==(const Listener& other) const {
  return (address_ == other.address_);
}

void Listener::addServer(const Server& server) {
  if (!servers_.insert(server).second) {
    std::cerr << "Server not added due to ip/port/server_name duplicate\n";
  }
}

void Listener::setEpollfd(int fd) {
  epoll_fd_ = fd;
}

void Listener::acceptConnection(int event) {
  std::cout << "Received event type " << event << "\n";
  Connection* c = new Connection(socket_fd_, epoll_fd_, servers_);
  try {
    connections_.push_back(c);
    std::cerr << "Successfully accepted connection\n";
  } catch (std::exception& e) {
    delete c;
  }
}

int Listener::getFd() const {
  return socket_fd_;
}
