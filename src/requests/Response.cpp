#include "Response.hpp"
#include <unistd.h>
#include <sstream>

Response::Response() : full_response_(""), fd_(-1), mode_(UNINITIALIZED) {}

Response::Response(int code,
                   const std::string& title,
                   const std::string& content) {
  std::ostringstream stream;

  stream << "HTTP/1.1 " << code << " " << title
         << "\r\nContent-Length: " << content.length() << "\r\n\r\n"
         << content;

  full_response_ = stream.str();
  mode_ = STATIC_CONTENT;
  fd_ = -1;
}

Response::Response(int code, const std::string& title, int fd) {
  std::ostringstream stream;

  stream << "HTTP/1.1 " << code << " " << title << "\r\nConnection: close\r\n";

  mode_ = FD;
  fd_ = fd;
  char buf[4096];
  int ret = read(fd, buf, 4096);
  if (ret == -1) {
    full_response_ = stream.str();
    full_response_ += "Content-Length: 5\r\n\r\n";
    full_response_ += "Error";
  } else {
    stream << "Content-Length: " << ret << "\r\n\r\n";
    full_response_ = stream.str();
    full_response_.append(buf, ret);
  }
}

Response::Response(const Response& other)
    : full_response_(other.full_response_),
      fd_(other.fd_),
      mode_(other.mode_) {}

Response& Response::operator=(const Response& other) {
  if (this != &other) {
    full_response_ = other.full_response_;
    mode_ = other.mode_;
    fd_ = other.fd_;
  }

  return *this;
}

Response::~Response() {}

const std::string& Response::getFullResponse() const {
  return full_response_;
}
