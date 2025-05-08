#include "Server.hpp"

Server::Server() {}

Server::Server(const Server& other) : server_name_(other.server_name_) {}

Server::~Server() {}

bool Server::operator<(const Server& other) const
{
  return server_name_ < other.server_name_;
}
