#include "Request.hpp"
#include <sys/socket.h>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include "../Connection.hpp"
#include "../utils/Utils.hpp"
#include "RequestStatus.hpp"

Request::Request(const int fd)
    : fd_(fd), status_(READING_START_LINE), closing_(false) {}

Request::Request(const Request& other)
    : fd_(other.fd_), status_(other.status_), closing_(other.closing_) {}

Request::~Request() {}

void Request::addHeaderLine(const std::string& line) {
  size_t pos = line.find('\r');
  if (pos != std::string::npos) {
    response_ = Response(400, "Bad Request", "400 Bad Request");
    status_ = SENDING_RESPONSE;
    return;
  }

  switch (status_) {
    case READING_START_LINE:
      return readStartLine(line);
    case READING_HEADERS:
      return parseHeaderLine(line);
    default:
      response_ = Response(200, "OK", line);
      status_ = SENDING_RESPONSE;
  }
}

void Request::sendResponse() {
  const std::string& response = response_.getFullResponse();
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

void Request::parseHeaderLine(const std::string& line) {
  std::string charset("!#$%&'*+-.^_`|~");
  typedef std::string::const_iterator iter_type;
  std::string name;
  std::string value;

  if (line.empty()) {
    response_ = Response(200, "OK", "YAY\r\n");
    status_ = SENDING_RESPONSE;
    return;
  }

  size_t pos = line.find(':');
  if (pos == std::string::npos) {
    throw std::runtime_error("Header does not contain a ':' character");
  }
  name = line.substr(0, pos);
  value = Utils::trimString(line.substr(pos + 1));

  for (iter_type it = name.begin(); it < name.end(); ++it) {
    if (charset.find_first_of(*it) == std::string::npos && !std::isalpha(*it) &&
        !std::isdigit(*it)) {
      throw std::runtime_error("Invalid character");
    }
  }

  std::cout << "Key: " << name << ", Value: " << value << "\n";
  headers_[name] = value;
}
