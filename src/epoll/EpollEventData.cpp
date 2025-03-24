#include "EpollEventData.hpp"

EpollEventData::~EpollEventData() {}

EpollFdTypes EpollEventData::getType() const {
  return fd_type_;
}

int EpollEventData::getFd() const {
  return fd_;
}
