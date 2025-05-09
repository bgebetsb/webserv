#pragma once

#include <sys/types.h>
#include <string>

enum ResponseMode
{
  UNINITIALIZED,
  STATIC_CONTENT,
  FD
};

class Response
{
 public:
  Response();
  Response(const std::string& filename);
  Response(int code, const std::string& title, const std::string& content);
  Response(int code, const std::string& title, const int fd);
  Response(const Response& other);
  Response& operator=(const Response& other);
  ~Response();

  const std::string& getFullResponse() const;

 private:
  std::string full_response_;
  int fd_;
  ResponseMode mode_;
  u_int16_t response_code_;
};
