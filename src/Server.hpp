#pragma once

#include <sys/types.h>
#include <string>
#include "Option.hpp"

using std::string;

class Server {
 public:
  Server();
  bool operator<(const Server& other) const;
  ~Server();

 private:
  Option< string > server_name_;
};
