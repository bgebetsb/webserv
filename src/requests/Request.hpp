#pragma once

#include <map>
#include <sstream>
#include <string>
#include "RequestStatus.hpp"
#include "requests/RequestMethods.hpp"

class Request {
 public:
  Request(const int fd);
  Request(const Request& other);
  ~Request();

  void addHeaderLine(const std::string& line);
  void sendResponse();
  RequestStatus getStatus() const;

 private:
  // Unused
  Request& operator=(const Request& other);

  const int fd_;
  RequestStatus status_;
  RequestMethod method_;
  std::string path_;
  std::map< std::string, std::string > headers_;

  void readStartLine(const std::string& line);
  void parseMethod(std::istringstream& stream);
  void parsePath(std::istringstream& stream);
  void parseHTTPVersion(std::istringstream& stream);
  void parseHeaderLine(const std::string& line);
};
