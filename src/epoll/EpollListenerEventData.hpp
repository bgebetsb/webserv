#pragma once

#include "Listener.hpp"
#include "epoll/EpollEventData.hpp"

class EpollListenerEventData : public EpollEventData {
 public:
  EpollListenerEventData(int fd, Listener& listener);
  ~EpollListenerEventData();
  void callback(int event) const;

 private:
  Listener& listener_;
};
