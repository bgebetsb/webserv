#include "Response.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ostream>
#include <sstream>
#include "epoll/Connection.hpp"
#include "exceptions/ConError.hpp"

Response::Response(int client_fd, int response_code, bool close_connection)
    : client_fd_(client_fd),
      response_code_(response_code),
      close_connection_(close_connection),
      complete_(false)
{
  switch (response_code)
  {
    case 200:
      response_title_ = "OK";
      break;
    case 204:
      response_title_ = "No Content";
      break;
    case 301:
      response_title_ = "Moved Permanently";
      break;
    case 302:
      response_title_ = "Found";
      break;
    case 303:
      response_title_ = "See Other";
      break;
    case 307:
      response_title_ = "Temporary Redirect";
      break;
    case 308:
      response_title_ = "Permanent Redirect";
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
    case 409:
      response_title_ = "Conflict";
      close_connection_ = true;
      break;
    case 411:
      response_title_ = "Length Required";
      close_connection_ = true;
      break;
    case 413:
      response_title_ = "Payload Too Large";
      close_connection_ = true;
      break;
    case 414:
      response_title_ = "URI Too Long";
      close_connection_ = true;
      break;
    case 500:
      response_title_ = "Internal Server Error";
      break;
    case 503:
      response_title_ = "Service unavailable";
      close_connection_ = true;
      break;
    case 505:
      response_title_ = "HTTP Version Not Supported";
      close_connection_ = true;
      break;
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

u_int16_t Response::getResponseCode() const
{
  return response_code_;
}

void Response::sendResponse()
{
  const char* buf;
  size_t amount;
  ssize_t ret;

  buf = full_response_.c_str();
  amount = std::min(static_cast< size_t >(CHUNK_SIZE), full_response_.length());

  ret = send(client_fd_, buf, amount, 0);
  if (ret == -1)
    throw ConErr("Peer closed connection");
  else if (ret == 0)
    throw ConErr("Send returned 0?!");

  full_response_ = full_response_.substr(ret);
  if (full_response_.empty())
    complete_ = true;
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
