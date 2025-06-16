#include "Response.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <ostream>
#include <sstream>
#include "../epoll/Connection.hpp"
#include "../exceptions/ConError.hpp"

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
    case 201:
      response_title_ = "Created";
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
      close_connection_ = true;
      break;
    case 501:
      response_title_ = "Not Implemented";
      close_connection_ = true;
      break;
    case 503:
      response_title_ = "Service unavailable";
      close_connection_ = true;
      break;
    case 504:
      response_title_ = "Gateway Timeout";
      close_connection_ = true;
      break;
    case 505:
      response_title_ = "HTTP Version Not Supported";
      close_connection_ = true;
      break;
    case 507:
      response_title_ = "Insufficient Storage";
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

std::string Response::createGenericResponseLines(void) const
{
  std::ostringstream stream;

  stream << "HTTP/1.1 " << response_code_ << " " << response_title_ << "\r\n";
  addDateHeader(stream);
  stream << "Connection: ";
  if (close_connection_)
    stream << "close\r\n";
  else
    stream << "keep-alive\r\n";
  return stream.str();
}

void Response::addDateHeader(std::ostringstream& stream) const
{
  std::time_t current = std::time(NULL);

  if (current == static_cast< std::time_t >(-1))
    return;
  std::tm* gmt = std::gmtime(&current);

  if (!gmt)
    return;

  char buffer[30];
  std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);

  stream << "Date: " << buffer << "\r\n";
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

bool Response::isCgiAndEmpty() const
{
  return false;
}

int Response::getClientFd() const
{
  return client_fd_;
}
