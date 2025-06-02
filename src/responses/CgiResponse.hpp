#pragma once

#include "epoll/PipeFd.hpp"
#include "requests/Request.hpp"
#include "responses/Response.hpp"

class CgiResponse : public Response
{
 public:
  CgiResponse(int client_fd,
              bool close,
              const std::string& cgi_path,
              const std::string& script_path,
              const std::string& file_path);
  ~CgiResponse();

  void sendResponse(void);

 private:
  PipeFd* pipe_fd_;
  bool headers_created_;
  mHeader headers_;
  bool status_found_;

  CgiResponse(const CgiResponse& other);
  CgiResponse& operator=(const CgiResponse& other);

  void processBuffer(void);
  void addHeaderLine(const std::string& line);
};
