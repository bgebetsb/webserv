#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
#include "Configs/Configs.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollFd.hpp"
#include "requests/Request.hpp"

#define CHUNK_SIZE 4096

class Connection : public EpollFd
{
 public:
  Connection(int socket_fd, const std::vector< Server >& servers);
  ~Connection();
  EpollAction epollCallback(int event);
  u_int64_t ping();

 private:
  const std::vector< Server >& servers_;
  char* readbuf_;
  std::string buffer_;
  Request request_;
  size_t request_timeout_ping_;
  size_t keepalive_last_ping_;

  Connection(const Connection& other);
  Connection& operator=(const Connection& other);

  EpollAction handleRead();
  EpollAction processBuffer();
  EpollAction handleWrite();
};
