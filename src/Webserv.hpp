#pragma once

#include <map>
#include <vector>
#include "Configs/Configs.hpp"
#include "Listener.hpp"
#include "epoll/EpollFd.hpp"
#include "ip/IpAddress.hpp"
#include "ip/IpComparison.hpp"

typedef int filedescriptor;
typedef std::set< IpAddress* > IpSet;
typedef std::map< const IpAddress*, filedescriptor, IpComparison > ListenerMap;
typedef std::map< const filedescriptor, EpollFd* > EpollMap;
typedef std::vector< Server > VServers;

class Webserv
{
 public:
  Webserv(std::string config_file = "webserv.conf");
  ~Webserv();

  void initialize_servas();
  void addServer(const IpSet& listeners, const Server& server);
  void mainLoop();

 private:
  ListenerMap listeners_;
  EpollMap fds_;
  const int epoll_fd_;
  VServers servers_;
  Configuration config_;

  // Copy constructor and copy assignment are unused anyway, thus private
  Webserv(const Server& other);
  Webserv& operator=(const Webserv& other);

  Listener& getListener(IpAddress* addr);
  void addFdsToEpoll() const;
  void addFd(int fd, struct epoll_event* event);
  void modifyFd(int fd, struct epoll_event* event) const;
  void deleteFd(int fd);
};
