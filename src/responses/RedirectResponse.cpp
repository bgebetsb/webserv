#include "RedirectResponse.hpp"
#include <sstream>

RedirectResponse::RedirectResponse(int client_fd,
                                   int response_code,
                                   const std::string& redirect_location,
                                   bool close)
    : Response(client_fd, response_code, close)
{
  std::ostringstream response;
  response << createResponseHeaderLine() << "Location: " << redirect_location
           << "\r\nConnection: ";
  if (close_connection_)
    response << "close\r\n";
  else
    response << "keep-alive\r\n";
  response << "Content-Length: 0\r\n\r\n";

  full_response_ = response.str();
}

RedirectResponse::~RedirectResponse() {}
