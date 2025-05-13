#include "RedirectResponse.hpp"
#include <sstream>

RedirectResponse::RedirectResponse(int client_fd,
                                   const std::string& redirect_location)
    : Response(client_fd, 301)
{
  std::ostringstream response;
  response << createResponseHeaderLine() << "Location: " << redirect_location
           << "\r\nConnection: ";
  response << "close\r\nContent-Length: 0\r\n\r\n";

  full_response_ = response.str();
}

RedirectResponse::~RedirectResponse() {}
