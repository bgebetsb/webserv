#pragma once

#include "Response.hpp"

class StaticResponse : public Response
{
 public:
  StaticResponse(int client_fd, int response_code, bool close);
  StaticResponse(int client_fd,
                 int response_code,
                 bool close,
                 const std::string& content);
  ~StaticResponse();

 private:
  StaticResponse(const StaticResponse& other);
  StaticResponse& operator=(const StaticResponse& other);
};
