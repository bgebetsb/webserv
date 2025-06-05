#pragma once

#include "epoll/EpollFd.hpp"
#include "requests/Request.hpp"
#include "responses/Response.hpp"
class CgiResponse : public Response
{
 public:
  CgiResponse(int client_fd,
              bool close,
              const std::string& cgi_path,
              const std::string& script_path,
              const std::string& file_path,
              const std::string& method,
              const std::string& query_string,
              long file_size);
  ~CgiResponse();

  void sendResponse(void);
  void unsetPipeFd(void);
  bool getHeadersCreated(void) const;

 private:
  EpollFd* pipe_fd_;
  bool headers_created_;
  mHeader headers_;
  bool status_found_;
  int file_size_;
  char** meta_variables_;
  const std::string& cgi_path_;
  const std::string& script_path_;
  const std::string& file_path_;
  const std::string& method_;
  const std::string& query_string_;
  bool last_chunk_sent_;
  CgiResponse(const CgiResponse& other);
  CgiResponse& operator=(const CgiResponse& other);
  char** implementMetaVariables();
  void processBuffer(void);
  void addHeaderLine(const std::string& line);
  void deleteMetaVariables(void);
};
