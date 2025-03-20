#pragma once

#include <vector>
#include "Listener.hpp"
#include "Server.hpp"

class Webserv {
 public:
  Webserv();
  ~Webserv();

  void addServer(const std::vector< Listener >& listeners,
                 const Server& server);
  void startListeners();

 private:
  std::vector< Listener > listeners_;
  const int epoll_fd_;
};
