#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "../Server.hpp"
#include "../responses/Response.hpp"
#include "RequestMethods.hpp"
#include "RequestStatus.hpp"

typedef std::map< std::string, std::string > mHeader;

class Request
{
 public:
  Request(const int fd, const std::vector< Server >& servers);
  Request(const Request& other);
  ~Request();

  void addHeaderLine(const std::string& line);
  void sendResponse();
  RequestStatus getStatus() const;
  bool closingConnection() const;
  void timeout();

 private:
  // Unused
  Request& operator=(const Request& other);

  const int fd_;
  RequestStatus status_;
  RequestMethod method_;
  std::string path_;
  mHeader headers_;
  bool closing_;
  const std::vector< Server >& servers_;
  Response* response_;

  void readStartLine(const std::string& line);
  void parseMethod(std::istringstream& stream);
  void parsePath(std::istringstream& stream);
  void parseHTTPVersion(std::istringstream& stream);
  void parseHeaderLine(const std::string& line);
  void processHeaders(void);
};
