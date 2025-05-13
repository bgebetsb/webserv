#include "../responses/StaticResponse.hpp"
#include "PathValidation/PathValidation.hpp"
#include "Request.hpp"
#include "exceptions/ConError.hpp"
#include "requests/RequestStatus.hpp"

void Request::readStartLine(const std::string& line)
{
  std::istringstream stream(line);

  parseMethod(stream);
  parsePath(stream);
  parseHTTPVersion(stream);

  if (status_ == READING_START_LINE)
  {
    status_ = READING_HEADERS;
  }
}

void Request::parseMethod(std::istringstream& stream)
{
  std::string method;

  if (!(stream >> method))
  {
    throw ConErr("Unable to parse method");
  }

  if (method == "GET")
  {
    method_ = GET;
  } else if (method == "POST")
  {
    method_ = POST;
  } else if (method == "DELETE")
  {
    method_ = DELETE;
  } else
  {
    method_ = INVALID;
  }
}

void Request::parsePath(std::istringstream& stream)
{
  std::string path;

  if (!(stream >> path))
  {
    throw ConErr("Unable to parse path");
  }

  path_ = preventEscaping(path);
}

void Request::parseHTTPVersion(std::istringstream& stream)
{
  std::string version;

  if (!(stream >> version))
  {
    throw ConErr("Unable to parse HTTP version");
  }

  if (version.length() != 8 || version.substr(0, 5) != "HTTP/" ||
      version[6] != '.' || !std::isdigit(version[5]) ||
      !std::isdigit(version[7]))
  {
    throw ConErr("Malformed HTTP version");
  }

  if (version != "HTTP/1.1")
  {
    response_ = new StaticResponse(fd_, 505);
    status_ = SENDING_RESPONSE;
  }
}
