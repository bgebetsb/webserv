#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include "Configs/Configs.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollFd.hpp"
#include "requests/Request.hpp"

#ifndef CHUNK_SIZE
#  define CHUNK_SIZE 4096
#endif

#define REQUEST_TIMEOUT_SECONDS 30
#define SEND_RECEIVE_TIMEOUT 60

class Connection : public EpollFd
{
 public:
  Connection(const std::vector< Server >& servers);
  virtual ~Connection() = 0;
  EpollAction epollCallback(int event);
  std::pair< EpollAction, u_int64_t > ping();

 protected:
  Request request_;
  std::string client_ip_;

 private:
  const std::vector< Server >& servers_;
  char* readbuf_;
  std::string buffer_;
  bool polling_write_;
  size_t request_timeout_ping_;
  size_t keepalive_last_ping_;
  size_t send_receive_ping_;

  // ── ◼︎ File Upload ───────────────────────
  long max_body_size_;
  long content_length_;
  long total_written_bytes_;
  bool chunked_;
  UploadMode mode_;

  Connection(const Connection& other);
  Connection& operator=(const Connection& other);

  EpollAction handleRead();
  EpollAction processBuffer();
  EpollAction processFileUpload();
  EpollAction handleWrite();
};
