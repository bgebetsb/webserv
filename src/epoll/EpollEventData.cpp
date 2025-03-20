#include "EpollEventData.hpp"

EpollEventData::EpollEventData(int fd,
                               EpollFdTypes type,
                               Connection* connection)
    : fd_(fd), fd_type_(type), connection_(connection) {}

EpollEventData::~EpollEventData() {}

EpollFdTypes EpollEventData::getType() const {
  return fd_type_;
}
