#pragma once

#include <map>
#include <vector>
#include "Listener.hpp"
#include "Server.hpp"
#include "epoll/EpollFd.hpp"

class Webserv {
 public:
  Webserv();
  ~Webserv();

  void addServer(const std::vector< Listener >& listeners,
                 const Server& server);
  void startListeners();
  void addFd(int fd, struct epoll_event* event);
  void modifyFd(int fd, struct epoll_event* event) const;
  void deleteFd(int fd);

 private:
  std::vector< Listener > listeners_;
  std::map< int, EpollFd* > fds_;
  const int epoll_fd_;
};
