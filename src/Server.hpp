#pragma once

#include <sys/types.h>
#include <string>
#include "Option.hpp"

using std::string;

class Server
{
 public:
  Server();
  Server(const Server& other);
  ~Server();

  bool operator<(const Server& other) const;

 private:
  Option< string > server_name_;
  Server& operator=(const Server& other);
  // ── ◼︎ CONFIG ───────────────────────
};
