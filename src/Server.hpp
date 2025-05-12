#pragma once

#include <sys/types.h>
#include <string>
#include <vector>
#include "Configs/Configs.hpp"

typedef std::string string;

typedef std::vector< string > host_vec;

struct serv_config;
class Server
{
 public:
  Server();
  Server(const Server& other);
  ~Server();

  const serv_config& getServerConfigs() const;

 private:
  // ── ◼︎ disabled ───────────────────────
  Server& operator=(const Server& other);

  // ── ◼︎ CONFIG ───────────────────────
  serv_config config_;
  int port_;
};
