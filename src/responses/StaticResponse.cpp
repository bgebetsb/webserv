#include "StaticResponse.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>
#include "responses/Response.hpp"

StaticResponse::StaticResponse(int client_fd, int response_code, bool close)
    : Response(client_fd, response_code, close)
{
  std::string content("<html><head><title>" + response_title_ +
                      "</title></head><body><h1>" + response_title_ +
                      "</h1></body></html>\r\n");

  std::ostringstream response;
  response << createResponseHeaderLine()
           << "Content-Length: " << content.length()
           << "\r\nContent-Type: text/html\r\nConnection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";
  response << content;

  full_response_ = response.str();
}

StaticResponse::StaticResponse(int client_fd,
                               int response_code,
                               bool close,
                               const std::string& content)
    : Response(client_fd, response_code, close)
{
  std::ostringstream response;
  response << createResponseHeaderLine()
           << "Content-Length: " << content.length() << "\r\n";
  if (!content.empty())
    response << "Content-Type: text/html\r\n";
  response << "Connection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";
  response << content;

  full_response_ = response.str();
}

StaticResponse::~StaticResponse() {}
