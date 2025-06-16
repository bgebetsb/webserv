#include "RedirectResponse.hpp"
#include <sstream>

RedirectResponse::RedirectResponse(int client_fd,
                                   int response_code,
                                   const std::string& redirect_location,
                                   bool close)
    : Response(client_fd, response_code, close)
{
  std::ostringstream response;
  response << createGenericResponseLines() << "Location: " << redirect_location
           << "\r\nContent-Length: 0\r\n\r\n";

  full_response_ = response.str();
}

RedirectResponse::~RedirectResponse() {}
