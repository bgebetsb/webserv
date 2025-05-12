#include "StaticResponse.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <sstream>
#include "../exceptions/ConError.hpp"
#include "Connection.hpp"
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

void StaticResponse::sendResponse(void)
{
  const char* buf;
  size_t amount;
  ssize_t ret;

  buf = full_response_.c_str();
  amount = std::min(static_cast< size_t >(CHUNK_SIZE), full_response_.length());

  ret = send(client_fd_, buf, amount, 0);
  if (ret == -1)
    throw ConErr("Send failed");

  full_response_ = full_response_.substr(ret);
  if (full_response_.empty())
    complete_ = true;
}
