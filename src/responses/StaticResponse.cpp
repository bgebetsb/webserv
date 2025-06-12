#include "StaticResponse.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <map>
#include <sstream>
#include "../responses/Response.hpp"

StaticResponse::StaticResponse(int client_fd, int response_code, bool close)
    : Response(client_fd, response_code, close)
{
  std::string content("<!DOCTYPE html>\r\n<html>\r\n<head>\r\n<title>" +
                      response_title_ +
                      "</title>\r\n</head>\r\n<body>\r\n<h1>" +
                      response_title_ + "</h1>\r\n</body>\r\n</html>\r\n");

  std::ostringstream response;
  response << createResponseHeaderLine()
           << "Content-Length: " << content.length()
           << "\r\nContent-Type: text/html; charset=utf-8\r\nConnection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";
  response << content;

  full_response_ = response.str();
}

StaticResponse::StaticResponse(
    int client_fd,
    int response_code,
    bool close,
    const std::string& content,
    const std::map< std::string, std::string > addHeaders)
    : Response(client_fd, response_code, close)
{
  std::ostringstream response;
  response << createResponseHeaderLine()
           << "Content-Length: " << content.length() << "\r\n";
  if (!content.empty())
    response << "Content-Type: text/html\r\n";
  for (std::map< std::string, std::string >::const_iterator it =
           addHeaders.begin();
       it != addHeaders.end(); ++it)
  {
    response << it->first << ": " << it->second << "\r\n";
  }
  response << "Connection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";
  response << content;
  full_response_ = response.str();
}

StaticResponse::~StaticResponse() {}
