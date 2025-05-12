#include "Server.hpp"
#include "Configs/Configs.hpp"

Server::Server() {}

Server::Server(const Server& other) : config_(other.config_) {}

Server::~Server() {}

const serv_config& Server::getServerConfigs(void) const
{
  return config_;
}
