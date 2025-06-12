#pragma once

#include <sys/types.h>
#include <map>
#include <string>
#include "Response.hpp"

typedef std::map< std::string, std::string > MMimeTypes;

class FileResponse : public Response
{
 public:
  FileResponse(int client_fd,
               const std::string& filename,
               int response_code = 200,
               bool close = false);
  ~FileResponse();

  void sendResponse(void);
  static const MMimeTypes mime_types_;

 private:
  int file_fd_;
  bool headers_created_;
  off_t remaining_;
  char* rd_buf_;
  bool eof_;
  std::string content_type_;

  FileResponse(const FileResponse& other);
  FileResponse& operator=(const FileResponse& other);

  void openFile(const std::string& filename);
  void createHeaders();
  std::string detectContentType(const std::string& filename) const;
};
