#pragma once

#include "Connection.hpp"

class Ipv4Connection : public Connection
{
 public:
  Ipv4Connection(int socket_fd, const std::vector< Server >& servers);
  ~Ipv4Connection();
};
