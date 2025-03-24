#include "EpollConnectionEventData.hpp"
#include <iostream>

EpollConnectionEventData::EpollConnectionEventData(int fd, Connection& listener)
    : connection_(listener) {
  fd_ = fd;
}

EpollConnectionEventData::~EpollConnectionEventData() {}

void EpollConnectionEventData::callback(int event) const {
  std::cout << "Received event type " << event << " on connection\n";
  // connection_.acceptConnection(event);
}
