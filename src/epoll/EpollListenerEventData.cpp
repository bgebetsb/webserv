#include "EpollListenerEventData.hpp"

EpollListenerEventData::EpollListenerEventData(int fd, Listener& listener)
    : listener_(listener) {
  fd_ = fd;
}

EpollListenerEventData::~EpollListenerEventData() {}

void EpollListenerEventData::callback(int event) const {
  listener_.acceptConnection(event);
}
