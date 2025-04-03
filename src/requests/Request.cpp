#include "Request.hpp"
#include <sys/socket.h>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../Connection.hpp"
#include "../utils/Utils.hpp"
#include "RequestStatus.hpp"

Request::Request(const int fd) : fd_(fd), status_(READING_START_LINE) {}

Request::Request(const Request& other)
    : fd_(other.fd_), status_(other.status_) {}

Request::~Request() {}

void Request::addHeaderLine(const std::string& line) {
  switch (status_) {
    case READING_START_LINE:
      return readStartLine(line);
    case READING_HEADERS:
      return parseHeaderLine(line);
    default:
      std::cout << "Line: " << line;
      status_ = SENDING_RESPONSE;
  }
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

void Request::readStartLine(const std::string& line) {
  std::istringstream stream(line);

  parseMethod(stream);
  parsePath(stream);
  parseHTTPVersion(stream);

  status_ = READING_HEADERS;
}

void Request::parseMethod(std::istringstream& stream) {
  std::string method;

  if (!(stream >> method)) {
    throw std::runtime_error("Unable to parse method");
  }

  if (method == "GET") {
    method_ = GET;
  } else if (method == "POST") {
    method_ = POST;
  } else if (method == "DELETE") {
    method_ = DELETE;
  } else {
    method_ = INVALID;
  }
}

void Request::parsePath(std::istringstream& stream) {
  if (!(stream >> path_)) {
    throw std::runtime_error("Unable to parse path");
  }
}

void Request::parseHTTPVersion(std::istringstream& stream) {
  std::string version;

  if (!(stream >> version)) {
    throw std::runtime_error("Unable to parse HTTP version");
  }

  if (version != "HTTP/1.1") {
    throw std::runtime_error("Invalid HTTP version");
  }
}

void Request::parseHeaderLine(const std::string& line) {
  std::string charset("!#$%&'*+-.^_`|~");
  typedef std::string::const_iterator iter_type;
  std::string name;
  std::string value;

  if (line == "\n" || line == "\r\n") {
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

  headers_[name] = value;
}
