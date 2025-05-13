#include "Response.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <ostream>
#include <sstream>

Response::Response(int client_fd, int response_code)
    : client_fd_(client_fd),
      response_code_(response_code),
      close_connection_(false),
      complete_(false)
{
  switch (response_code)
  {
    case 200:
      response_title_ = "OK";
      break;
    case 301:
      response_title_ = "Moved Permanently";
      break;
    case 400:
      response_title_ = "Bad Request";
      close_connection_ = true;
      break;
    case 403:
      response_title_ = "Forbidden";
      break;
    case 404:
      response_title_ = "Not Found";
      break;
    case 405:
      response_title_ = "Method Not Allowed";
      break;
    case 408:
      response_title_ = "Request Timeout";
      close_connection_ = true;
      break;
    case 500:
      response_title_ = "Internal Server Error";
      break;
    // TODO: If I add new error here, also add them to Configs.hpp
    default:
      response_title_ = "Not implemented";
  }
}

Response::~Response() {}

void Response::setCloseConnectionHeader(void)
{
  close_connection_ = true;
}

bool Response::isComplete() const
{
  return complete_;
}

std::string Response::createResponseHeaderLine(void) const
{
  std::ostringstream stream;

  stream << "HTTP/1.1 " << response_code_ << " " << response_title_ << "\r\n";
  return stream.str();
}

bool Response::getClosing() const
{
  return close_connection_;
}

// Response::Response() : client_fd_(-1), file_fd_(-1), mode_(UNINITIALIZED) {}
//
// Response::Response(int client_fd, const std::string& filename)
//     : client_fd_(client_fd), file_fd_(-1)
// {
//   PathInfos infos;
//
//   infos = getFileType(filename);
//
//   if (!infos.exists)
//   {
//     response_code_ = 404;
//   } else if (!infos.readable || infos.types == OTHER)
//   {
//     response_code_ = 403;
//   } else
//   {
//     file_fd_ = open(filename.c_str(), O_RDONLY);
//     if (file_fd_ == -1)
//       response_code_ = 500;
//     else
//       response_code_ = 200;
//   }
//
//   if (file_fd_ == -1)
//     mode_ = STATIC_CONTENT;
//   else
//     mode_ = FD;
// }
//
// Response::Response(int code,
//                    const std::string& title,
//                    const std::string& content)
// {
//   std::ostringstream stream;
//
//   stream << "HTTP/1.1 " << code << " " << title
//          << "\r\nContent-Length: " << content.length() << "\r\n\r\n"
//          << content;
//
//   full_response_ = stream.str();
//   mode_ = STATIC_CONTENT;
//   fd_ = -1;
// }
//
// Response::Response(int code, const std::string& title, int fd)
// {
//   std::ostringstream stream;
//
//   stream << "HTTP/1.1 " << code << " " << title << "\r\nConnection:
//   close\r\n";
//
//   mode_ = FD;
//   fd_ = fd;
//   char buf[4096];
//   int ret = read(fd, buf, 4096);
//   if (ret == -1)
//   {
//     full_response_ = stream.str();
//     full_response_ += "Content-Length: 5\r\n\r\n";
//     full_response_ += "Error";
//   } else
//   {
//     stream << "Content-Length: " << ret << "\r\n\r\n";
//     full_response_ = stream.str();
//     full_response_.append(buf, ret);
//   }
// }
//
// Response::Response(const Response& other)
//     : full_response_(other.full_response_), fd_(other.fd_),
//     mode_(other.mode_)
// {}
//
// Response& Response::operator=(const Response& other)
// {
//   if (this != &other)
//   {
//     full_response_ = other.full_response_;
//     mode_ = other.mode_;
//     fd_ = other.fd_;
//   }
//
//   return *this;
// }
//
// Response::~Response() {}
//
// const std::string& Response::getFullResponse() const
// {
//   return full_response_;
// }
