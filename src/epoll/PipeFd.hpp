#pragma once

#include <string>
#include "EpollFd.hpp"
#include "epoll/Connection.hpp"
#include "responses/Response.hpp"

#ifndef CHUNK_SIZE
#  define CHUNK_SIZE 4096
#endif

class PipeFd : public EpollFd
{
 public:
  // ── ◼︎ constructor, destructor ───────────────────────
  PipeFd(std::string& write_buffer,
         const std::string& skript_path,
         const std::string& cgi_path,
         const std::string& file_path,
         Response* cgi_response,
         char** envp);
  ~PipeFd();

  EpollAction epollCallback(int event);

 private:
  // ── ◼︎ member variables ───────────────────────
  int read_end_;
  int write_end_;
  int process_id_;
  bool process_finished_;
  char read_buffer_[CHUNK_SIZE];
  std::string& write_buffer_;
  std::string bin_path_;
  std::string skript_path_;
  std::string file_path_;
  Response* cgi_response_;
  size_t start_time_;
  // ── ◼︎ utils ───────────────────────
  void closePipe();
  void spawnCGI(char** envp);

  // ── ◼︎ disabled ───────────────────────
  PipeFd();
  PipeFd(const PipeFd& other);
  PipeFd& operator=(const PipeFd& other);
};
