#pragma once

#include <sys/types.h>
#include "responses/Response.hpp"
class FileResponse : public Response
{
 public:
  FileResponse(int client_fd,
               const std::string& filename,
               int response_code = 200,
               bool close = false);
  ~FileResponse();

  void sendResponse(void);

 private:
  int file_fd_;
  bool headers_created_;
  off_t remaining_;
  char* rd_buf_;
  bool eof_;

  FileResponse(const FileResponse& other);
  FileResponse& operator=(const FileResponse& other);

  void openFile(const std::string& filename);
  void createHeaders();
};
