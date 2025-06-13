#include <sys/types.h>
#include <cctype>
#include <sstream>
#include <string>
#include "../exceptions/RequestError.hpp"
#include "../parsing/Parsing.hpp"
#include "Request.hpp"
#include "RequestStatus.hpp"

static void parseHTTPVersion(std::istringstream& stream);

void Request::readStartLine(const std::string& line)
{
  if (line.empty())
    return;
  startline_ = line;

  std::istringstream stream(line);

  parseMethod(stream);
  Parsing::skip_character(stream, ' ');
  parsePath(stream);
  Parsing::skip_character(stream, ' ');
  parseHTTPVersion(stream);

  status_ = READING_HEADERS;
}

void Request::parseMethod(std::istringstream& stream)
{
  std::string method;

  method = Parsing::get_token(stream);

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
  std::string::size_type pos;

  if (!(stream >> std::noskipws >> path_))
    throw RequestError(400, "Unable to parse path");
  pos = path_.find('?');
  if (pos != std::string::npos)
  {
    query_string_ = path_.substr(pos + 1);
    query_string_ = Parsing::processQueryString(query_string_);
    path_ = path_.substr(0, pos);
  }

  if (path_[0] != '/')
    parseAbsoluteForm(path_);
  else
    path_ = Parsing::processPath(path_);
}

void Request::parseAbsoluteForm(const std::string& abs_path)
{
  std::string delim = "://";
  size_t pos = abs_path.find(delim);

  if (pos == 0 || pos == std::string::npos || abs_path.substr(0, pos) != "http")
    throw RequestError(400, "Absolute form: http scheme not found");

  std::string path = abs_path.substr(pos + delim.length());

  pos = path.find('@');
  if (pos != std::string::npos)
  {
    Parsing::validateUserinfo(path.substr(0, pos));
    path = path.substr(pos + 1);
  }

  if (path.empty())
    throw RequestError(400, "absolute form: empty str after scheme+userinfo");

  pos = path.find('/');
  if (pos != std::string::npos)
  {
    host_ = path.substr(0, pos);
    path_ = Parsing::processPath(path.substr(pos));
  }
  else
  {
    host_ = path;
    path_ = "/";
  }

  std::pair< std::string, u_int16_t > authority = Parsing::parseHost(host_);
  host_ = authority.first;
  std::ostringstream ss;
  ss << authority.second;
  port_ = ss.str();
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

  stream.get();
  if (!stream.fail())
    throw RequestError(400, "Unexpected content after HTTP version");

  if (version != "HTTP/1.1")
    throw RequestError(505, "Invalid HTTP version");
}
