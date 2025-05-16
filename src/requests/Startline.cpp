#include <cctype>
#include <string>
#include "../responses/StaticResponse.hpp"
#include "PathValidation/PathValidation.hpp"
#include "Request.hpp"
#include "exceptions/ConError.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/RequestStatus.hpp"

void Request::readStartLine(const std::string& line)
{
  std::istringstream stream(line);

  parseMethod(stream);
  parsePath(stream);
  parseHTTPVersion(stream);

  if (status_ == READING_START_LINE)
    status_ = READING_HEADERS;
}

void Request::parseMethod(std::istringstream& stream)
{
  std::string method;

  if (!(stream >> method))
    throw ConErr("Unable to parse method");

  if (method == "GET")
    method_ = GET;
  else if (method == "POST")
    method_ = POST;
  else if (method == "DELETE")
    method_ = DELETE;
  else
    method_ = INVALID;
}

void Request::parsePath(std::istringstream& stream)
{
  std::string::iterator it;

  if (!(stream >> path_))
    throw RequestError(400, "Unable to parse path");

  if (path_[0] != '/')
  {
    if (!parseAbsoluteForm(path_))
      throw RequestError(400, "Parsing absolute form failed");
  }

  path_ = preventEscaping(path_);
}

bool Request::parseAbsoluteForm(const std::string& abs_path)
{
  std::string delim = "://";
  size_t pos = abs_path.find(delim);

  if (pos == 0 || pos == std::string::npos ||
      !validateScheme(abs_path.substr(0, pos)))
  {
    return (false);
  }

  std::string path = abs_path.substr(pos + delim.length());
  if (path.empty())
    return (false);

  pos = path.find('/');
  if (pos != std::string::npos)
  {
    host_ = path.substr(0, pos);
    path_ = path.substr(pos);
  }
  else
  {
    host_ = path;
    path_ = "/";
  }

  pos = host_.find(':');
  if (pos == std::string::npos)
    host_ = host_.substr(0, pos);

  return (true);
}

bool Request::validateScheme(const std::string& scheme) const
{
  std::string::const_iterator it;

  for (it = scheme.begin(); it != scheme.end(); ++it)
  {
    if (!std::isalpha(*it))
      return (false);
  }

  return (true);
}

void Request::parseHTTPVersion(std::istringstream& stream)
{
  std::string version;

  if (!(stream >> version))
    throw RequestError(400, "Unable to parse HTTP version");

  if (version.length() != 8 || version.substr(0, 5) != "HTTP/" ||
      version[6] != '.' || !std::isdigit(version[5]) ||
      !std::isdigit(version[7]))
  {
    throw RequestError(400, "Malformed HTTP version");
  }

  if (version != "HTTP/1.1")
    throw RequestError(505, "Invalid HTTP version");
}
