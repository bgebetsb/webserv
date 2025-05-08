#pragma once

#include <map>
#include <sstream>
#include <string>
#include "RequestMethods.hpp"
#include "RequestStatus.hpp"
#include "Response.hpp"

class Request {
 public:
  Request(const int fd);
  Request(const Request& other);
  ~Request();

  void addHeaderLine(const std::string& line);
  void sendResponse();
  RequestStatus getStatus() const;
  bool closingConnection() const;

 private:
  // Unused
  Request& operator=(const Request& other);

  const int fd_;
  RequestStatus status_;
  RequestMethod method_;
  std::string path_;
  std::map< std::string, std::string > headers_;
  Response response_;
  bool closing_;

  void readStartLine(const std::string& line);
  void parseMethod(std::istringstream& stream);
  void parsePath(std::istringstream& stream);
  void parseHTTPVersion(std::istringstream& stream);
  void parseHeaderLine(const std::string& line);
};
