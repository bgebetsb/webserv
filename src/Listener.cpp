#include "Listener.hpp"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <exception>
#include <iostream>
#include <stdexcept>
#include "Connection.hpp"
#include "epoll/EpollAction.hpp"
#include "ip/IpAddress.hpp"

Listener::Listener(IpAddress* address) : address_(address) {
  setup();
  if (::listen(fd_, 0) == -1) {
    throw std::runtime_error("Unable to listen");
  }

  std::cout << "Started listening\n";
}

Listener::~Listener() {}

void Listener::setup() {
  fd_ = address_->createSocket();
}

struct epoll_event* Listener::getEpollEvent() {
  return ep_event_;
}

bool Listener::operator==(const Listener& other) const {
  return (address_ == other.address_);
}

void Listener::addServer(const Server& server) {
  if (!servers_.insert(server).second) {
    std::cerr << "Server not added due to ip/port/server_name duplicate\n";
  }
}

EpollAction Listener::epollCallback(int event) {
  if (event & EPOLLRDHUP) {
    EpollAction action = {getFd(), EPOLL_ACTION_DEL, NULL};
    return action;
  }

  return acceptConnection();
}

EpollAction Listener::acceptConnection() {
  try {
    Connection* c = new Connection(fd_, servers_);
    std::cerr << "Successfully accepted connection\n";
    EpollAction action = {c->getFd(), EPOLL_ACTION_ADD, c->getEvent()};
    return action;
  } catch (std::exception& e) {
    throw;
  }
}
