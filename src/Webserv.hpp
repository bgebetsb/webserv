#pragma once

#include <vector>
#include "Listener.hpp"
#include "Server.hpp"

class Webserv {
 public:
  Webserv();
  void addServer(const std::vector< Listener >& listeners,
                 const Server& server);

 private:
  std::vector< Listener > listeners_;
};
