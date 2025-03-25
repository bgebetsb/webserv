#include "EpollFd.hpp"
#include <unistd.h>
#include <iostream>

EpollFd::EpollFd(Webserv& webserver) : fd_(-1), webserver_(webserver) {}

EpollFd::~EpollFd() {
  if (fd_ != -1) {
    std::cout << "Closing fd\n";
    close(fd_);
  }
}

int EpollFd::getFd() const {
  return fd_;
}
