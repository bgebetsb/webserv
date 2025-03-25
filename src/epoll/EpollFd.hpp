#pragma once

class Webserv;

class EpollFd {
 public:
  EpollFd(Webserv& webserver);
  virtual ~EpollFd();
  virtual void epollCallback(int event) = 0;
  int getFd() const;

 protected:
  int fd_;
  Webserv& webserver_;
};
