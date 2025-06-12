#include "Listener.hpp"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include "../exceptions/ConError.hpp"
#include "../exceptions/Fatal.hpp"
#include "../ip/IpAddress.hpp"
#include "../utils/Utils.hpp"
#include "Connection.hpp"
#include "EpollAction.hpp"
#include "Ipv4Connection.hpp"
#include "Ipv6Connection.hpp"

Listener::Listener(IpAddress* address) : address_(address)
{
  setup();

  if (Utils::addCloExecFlag(fd_) == -1)
  {
    close(fd_);
    throw Fatal("Unable to set O_CLOEXEC on Listener fd");
  }
  // Second argument specifies how many pending connections it should keep in
  // the backlog
  if (::listen(fd_, SOMAXCONN) == -1)
  {
    close(fd_);
    throw Fatal("Unable to listen");
  }

  std::cerr << "Started listening on " << *address << "\n";
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
    Connection* c;
    if (address_->getType() == IPv4)
      c = new Ipv4Connection(fd_, servers_);
    else
      c = new Ipv6Connection(fd_, servers_);
    if (Utils::addCloExecFlag(c->getFd()) == -1)
    {
      delete c;
      throw ConErr("Unable to set O_CLOEXEC on accepted client connection");
    }
    EpollAction action = {c->getFd(), EPOLL_ACTION_ADD, c->getEvent()};
    return action;
  }
  catch (ConErr& e)
  {
    std::cerr << e.what() << "\n";
    EpollAction action = {getFd(), EPOLL_ACTION_UNCHANGED, getEvent()};
    return action;
  }
}
