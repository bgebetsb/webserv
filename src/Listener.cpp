#include "Listener.hpp"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include "Connection.hpp"
#include "epoll/EpollAction.hpp"
#include "exceptions/ConError.hpp"
#include "ip/IpAddress.hpp"

Listener::Listener(IpAddress* address) : address_(address)
{
  setup();
  // Second argument specifies how many pending connections it should keep in
  // the backlog
  if (::listen(fd_, SOMAXCONN) == -1)
  {
    throw std::runtime_error("Unable to listen");
  }

  std::cerr << "Started listening\n";
}

Listener::~Listener() {}

void Listener::setup()
{
  fd_ = address_->createSocket();
}

struct epoll_event* Listener::getEpollEvent()
{
  return ep_event_;
}

bool Listener::operator==(const Listener& other) const
{
  return (address_ == other.address_);
}

void Listener::addServer(const Server& server)
{
  servers_.push_back(server);
}

EpollAction Listener::epollCallback(int event)
{
  if (event & EPOLLRDHUP)
  {
    EpollAction action = {getFd(), EPOLL_ACTION_DEL, NULL};
    return action;
  }

  return acceptConnection();
}

EpollAction Listener::acceptConnection()
{
  try
  {
    Connection* c = new Connection(fd_, servers_);
    EpollAction action = {c->getFd(), EPOLL_ACTION_ADD, c->getEvent()};
    return action;
  } catch (ConErr& e)
  {
    std::cerr << e.what() << "\n";
    EpollAction action = {getFd(), EPOLL_ACTION_UNCHANGED, getEvent()};
    return action;
  }
}
