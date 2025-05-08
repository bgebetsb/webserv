#pragma once

#include <sys/types.h>
#include <string>
#include <vector>
#include "Configs.hpp"
#include "Option.hpp"

typedef std::string string;

typedef std::vector< string > host_vec;
class Server
{
 public:
  Server();
  Server(const Server& other);
  ~Server();

  bool operator<(const Server& other) const;

 private:
  // ── ◼︎ disabled ───────────────────────
  Server& operator=(const Server& other);

  Option< string > server_name_;
  // ── ◼︎ CONFIG ───────────────────────
  serv_config config_;
  int port_;
};
