#pragma once

#include "Connection.hpp"

class Ipv6Connection : public Connection
{
 public:
  Ipv6Connection(int socket_fd, const std::vector< Server >& servers);
  ~Ipv6Connection();
};
