#include "PathValidation/PathValidation.hpp"
#include "Request.hpp"

void Request::readStartLine(const std::string& line)
{
  std::istringstream stream(line);

  parseMethod(stream);
  parsePath(stream);
  parseHTTPVersion(stream);

  status_ = READING_HEADERS;
}

void Request::parseMethod(std::istringstream& stream)
{
  std::string method;

  if (!(stream >> method))
  {
    throw std::runtime_error("Unable to parse method");
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
    throw std::runtime_error("Unable to parse path");
  }

  path_ = preventEscaping(path);
}

void Request::parseHTTPVersion(std::istringstream& stream)
{
  std::string version;

  if (!(stream >> version))
  {
    throw std::runtime_error("Unable to parse HTTP version");
  }

  if (version != "HTTP/1.1")
  {
    throw std::runtime_error("Invalid HTTP version");
  }
}
