#pragma once

#include <sys/types.h>
#include "responses/Response.hpp"
class FileResponse : public Response
{
 public:
  FileResponse(int client_fd, int file_fd, off_t size);
  ~FileResponse();

  void sendResponse(void);

 private:
  int file_fd_;
  off_t remaining_;
  char* rd_buf_;
  bool eof_;
};
