#pragma once

#include "Connection.hpp"
#include "epoll/EpollEventData.hpp"

class EpollConnectionEventData : public EpollEventData {
 public:
  EpollConnectionEventData(int fd, Connection& connection);
  ~EpollConnectionEventData();
  void callback(int event) const;

 private:
  Connection& connection_;
};
