#pragma once

#include <sys/types.h>
#include <string>

class Response
{
 public:
  Response(int client_fd, int response_code, bool close_connection);
  virtual ~Response();

  void setCloseConnectionHeader(void);
  virtual void sendResponse(void);
  bool isComplete(void) const;
  bool getClosing() const;

 protected:
  std::string full_response_;
  std::string response_title_;
  int client_fd_;
  u_int16_t response_code_;
  bool close_connection_;
  bool complete_;

  std::string createResponseHeaderLine(void) const;

 private:
  Response(const Response& other);
  Response& operator=(const Response& other);
};
