#pragma once

#include <map>
#include "Response.hpp"

class StaticResponse : public Response
{
 public:
  StaticResponse(int client_fd, int response_code, bool close);
  StaticResponse(int client_fd,
                 int response_code,
                 bool close,
                 const std::string& content,
                 std::map< std::string, std::string > additional_headers =
                     std::map< std::string, std::string >());
  ~StaticResponse();

 private:
  StaticResponse(const StaticResponse& other);
  StaticResponse& operator=(const StaticResponse& other);
};
