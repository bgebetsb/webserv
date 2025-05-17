#include <cctype>
#include <string>
#include "../responses/StaticResponse.hpp"
#include "PathValidation/PathValidation.hpp"
#include "Request.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/RequestStatus.hpp"

static bool validateScheme(const std::string& scheme);
static void parseHTTPVersion(std::istringstream& stream);

void Request::readStartLine(const std::string& line)
{
  if (line.empty())
    return;
  if (std::isspace(line[0]))
    throw RequestError(400, "Leading spaces in Start line");

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
    throw RequestError(400, "Unable to parse method");

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

static bool validateScheme(const std::string& scheme)
{
  std::string::const_iterator it;

  for (it = scheme.begin(); it != scheme.end(); ++it)
  {
    if (!std::isalpha(*it))
      return (false);
  }

  return (true);
}

static void parseHTTPVersion(std::istringstream& stream)
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
