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
#include "Webserv.hpp"
#include "ip/IpAddress.hpp"

Listener::Listener(Webserv& webserver, IpAddress* address)
    : EpollFd(webserver), address_(address) {}

Listener::~Listener() {}

void Listener::listen() {
  setup();
  if (::listen(fd_, 0) == -1) {
    throw std::runtime_error("Unable to listen");
  }

  std::cout << "Started listening\n";

  webserver_.addFd(fd_, ep_event_);
}

void Listener::setup() {
  fd_ = address_->createSocket();

  ep_event_ = new struct epoll_event();
  ep_event_->events = EPOLLIN | EPOLLRDHUP;
  // EpollEventData* data = new EpollListenerEventData(socket_fd_, *this);

  ep_event_->data.ptr = this;
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

void Listener::epollCallback(int event) {
  acceptConnection(event);
}

// void Listener::setEpollfd(int fd) {
//   epoll_fd_ = fd;
// }
//
void Listener::acceptConnection(int event) {
  std::cout << "Received event type " << event << "\n";
  Connection* c = new Connection(webserver_, fd_, servers_);
  try {
    // connections_.push_back(c);
    std::cerr << "Successfully accepted connection\n";
  } catch (std::exception& e) {
    delete c;
  }
}

// int Listener::getFd() const {
//   return socket_fd_;
// }
