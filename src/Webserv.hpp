#pragma once

#include <map>
#include <vector>
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
  void startListeners();
  void addFd(int fd, struct epoll_event* event);
  void modifyFd(int fd, struct epoll_event* event) const;
  void deleteFd(int fd);

 private:
  std::map< const IpAddress*, int, IpComparison > listeners_;
  std::map< const int, EpollFd* > fds_;
  const int epoll_fd_;
};
