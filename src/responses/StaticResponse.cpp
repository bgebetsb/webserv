#include "StaticResponse.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>
#include "responses/Response.hpp"

StaticResponse::StaticResponse(int client_fd, int response_code)
    : Response(client_fd, response_code)
{
  std::string content("<html><head><title>" + response_title_ +
                      "</title></head><body><h1>" + response_title_ +
                      "</h1></body></html>");

  std::ostringstream response;
  response << createResponseHeaderLine()
           << "Content-Length: " << content.length() << "\r\nConnection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";
  response << content;

  full_response_ = response.str();
}

StaticResponse::~StaticResponse() {}
