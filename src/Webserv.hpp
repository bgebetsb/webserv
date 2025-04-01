#pragma once

#include <map>
#include <vector>
#include "Listener.hpp"
#include "Server.hpp"
#include "epoll/EpollFd.hpp"
#include "ip/IpAddress.hpp"
#include "ip/IpComparison.hpp"

class Webserv {
 public:
  Webserv();
  ~Webserv();

  void addServer(const std::vector< IpAddress* >& listeners,
                 const Server& server);
  void mainLoop();

 private:
  std::map< const IpAddress*, int, IpComparison > listeners_;
  std::map< const int, EpollFd* > fds_;
  const int epoll_fd_;

  // Copy constructor and copy assignment are unused anyway, thus private
  Webserv(const Server& other);
  Webserv& operator=(const Webserv& other);

  Listener& getListener(IpAddress* addr);
  void addFdsToEpoll() const;
  void addFd(int fd, struct epoll_event* event);
  void modifyFd(int fd, struct epoll_event* event) const;
  void deleteFd(int fd);
};
