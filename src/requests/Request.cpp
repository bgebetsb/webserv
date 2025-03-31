#include "Request.hpp"
#include <sys/socket.h>
#include <iostream>
#include "../Connection.hpp"
#include "RequestStatus.hpp"

Request::Request(const int fd) : fd_(fd), status_(READING_HEADERS) {}

Request::Request(const Request& other) : fd_(other.fd_) {}

Request::~Request() {}

void Request::addHeaderLine(const std::string& line) {
  std::cout << "Line: " << line;
  status_ = SENDING_RESPONSE;
}

void Request::sendResponse() {
  std::string response("Full string received oida\r\n");
  ssize_t ret =
      send(fd_, response.c_str(),
           std::min(static_cast< size_t >(CHUNK_SIZE), response.size()), 0);
  if (ret == -1) {
    throw std::runtime_error("Send failed");
  }

  status_ = COMPLETED;
}

RequestStatus Request::getStatus() const {
  return status_;
}
