#pragma once

#include <string>
#include "RequestStatus.hpp"

class Request {
 public:
  Request(const int fd);
  Request(const Request& other);
  ~Request();

  void addHeaderLine(const std::string& line);
  void sendResponse();
  RequestStatus getStatus() const;

 private:
  Request& operator=(const Request& other);

  const int fd_;
  RequestStatus status_;
};
