#include "Request.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
#include "../utils/Utils.hpp"
#include "Configs/Configs.hpp"
#include "Option.hpp"
#include "PathValidation/FileTypes.hpp"
#include "PathValidation/PathInfos.hpp"
#include "PathValidation/PathValidation.hpp"
#include "RequestStatus.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/RequestMethods.hpp"
#include "responses/FileResponse.hpp"
#include "responses/Response.hpp"

Request::Request(const int fd, const std::vector< Server >& servers)
    : fd_(fd),
      status_(READING_START_LINE),
      chunked_(false),
      content_length_(Option< long >()),
      closing_(false),
      servers_(servers),
      total_header_size_(0),
      response_(NULL)
{}

Request::Request(const Request& other)
    : fd_(other.fd_),
      status_(other.status_),
      chunked_(other.chunked_),
      content_length_(other.content_length_),
      closing_(other.closing_),
      servers_(other.servers_),
      total_header_size_(other.total_header_size_),
      response_(other.response_)
{}

Request& Request::operator=(const Request& other)
{
  if (this != &other)
  {
    fd_ = other.fd_;
    status_ = other.status_;
    chunked_ = other.chunked_;
    content_length_ = other.content_length_;
    closing_ = other.closing_;
    total_header_size_ = other.total_header_size_;
    if (response_)
    {
      delete response_;
    }
    response_ = other.response_;
  }

  return *this;
}

Request::~Request()
{
  if (response_)
  {
    delete response_;
  }
}

void Request::addHeaderLine(const std::string& line)
{
  size_t pos = line.find('\r');
  if (pos != std::string::npos)
    throw RequestError(400, "Unexpected CR before end of header line");

  total_header_size_ += line.size();
  if (total_header_size_ > 32768)
    throw RequestError(400, "Total header size too large");

  switch (status_)
  {
    case READING_START_LINE:
      return readStartLine(line);
    case READING_HEADERS:
      return parseHeaderLine(line);
    default:
      // TODO: Figure out again what I was doing here lol
      // response_ = Response(200, "OK", line);
      // status_ = SENDING_RESPONSE;
      throw;
  }
}

void Request::sendResponse()
{
  response_->sendResponse();
  if (response_->isComplete())
  {
    status_ = COMPLETED;
    closing_ = response_->getClosing();
  }
}

RequestStatus Request::getStatus() const
{
  return status_;
}

void Request::parseHeaderLine(const std::string& line)
{
  std::string charset("!#$%&'*+-.^_`|~");
  typedef std::string::const_iterator iter_type;
  std::string name;
  std::string value;

  if (line.empty())
    return processHeaders();

  size_t pos = line.find(':');
  if (pos == std::string::npos)
    throw RequestError(400, "Header does not contain a ':' character");

  name = line.substr(0, pos);
  value = Utils::trimString(line.substr(pos + 1));

  std::for_each(name.begin(), name.end(), Utils::toLower);

  for (iter_type it = name.begin(); it < name.end(); ++it)
  {
    char c = *it;

    if (charset.find(c) == std::string::npos && !std::isalpha(c) &&
        !std::isdigit(c))
    {
      throw RequestError(400, "Invalid character in header name");
    }
  }

  insertHeader(name, value);
}

void Request::processHeaders(void)
{
  Option< std::string > host = getHeader("Host");

  if (host.is_none())
    throw RequestError(400, "Missing Host header");

  Option< std::string > transfer_encoding = getHeader("Transfer-Encoding");
  Option< std::string > content_length = getHeader("Content-Length");
  if (transfer_encoding.is_some())
  {
    validateTransferEncoding(transfer_encoding.unwrap());
    if (content_length.is_some())
      throw RequestError(400,
                         "Both Content-Length and Transfer-Encoding present");
  }
  else if (content_length.is_some())
  {
    validateContentLength(content_length.unwrap());
  }

  if (host_.empty())
    host_ = host.unwrap();

  const Server& server = getServer(host_);
  const location& location = findMatchingLocationBlock(server.locations, path_);

  PathInfos infos;

  if (!methodAllowed(location))
    throw RequestError(405, "Method now allowed");

  redirection redir = location.redirect;
  if (redir.has_been_set)
  {
    std::string location = redir.uri;
    replaceString(location, "$host", host_);
    replaceString(location, "$request_uri", path_);
    response_ = new RedirectResponse(fd_, redir.code, location);
    status_ = SENDING_RESPONSE;
    return;
  }

  if (location.root.empty())
    throw RequestError(404, "No root directory set for location");

  std::string full_path = location.root + path_;

  infos = getFileType(full_path);

  if (!infos.exists)
    throw RequestError(404, "File doesn't exist");
  else if (!infos.readable || infos.types == OTHER)
    throw RequestError(403, "File not readable or incorrect type");
  else
  {
    int fd = open(full_path.c_str(), O_RDONLY | O_NOFOLLOW);
    if (fd == -1)
      throw RequestError(500, "Open failed for unknown reason");
    else
    {
      response_ = new FileResponse(fd_, fd, infos.size);
      status_ = SENDING_RESPONSE;
    }
  }
}

bool Request::closingConnection() const
{
  return closing_;
}

// TODO: Maybe remove this and just use the setResponse function from outside
void Request::timeout()
{
  response_ = new StaticResponse(fd_, 408);
  status_ = SENDING_RESPONSE;
}

void Request::setResponse(Response* response)
{
  response_ = response;
  status_ = SENDING_RESPONSE;
}

Option< std::string > Request::getHeader(const std::string& name) const
{
  std::string lower(name);
  std::for_each(lower.begin(), lower.end(), Utils::toLower);

  mHeader::const_iterator it = headers_.find(lower);
  if (it == headers_.end())
  {
    return Option< std::string >();
  }

  return Option< std::string >(it->second);
}

const Server& Request::getServer(const std::string& host) const
{
  vServer::const_iterator server;

  for (server = servers_.begin(); server != servers_.end(); ++server)
  {
    if (server->server_names.find(host) != server->server_names.end())
      return *server;
  }

  return servers_.front();
}

void Request::replaceString(std::string& str,
                            const std::string& search,
                            const std::string& replace) const
{
  std::string::size_type pos = str.find(search);
  if (pos != std::string::npos)
    str = str.replace(pos, search.length(), replace);
}

bool Request::methodAllowed(const location& location) const
{
  switch (method_)
  {
    case GET:
      return location.GET;
    case POST:
      return location.POST;
    case DELETE:
      return location.DELETE;
    default:
      return false;
  }
}

/*
 * Key should already be lower-case here since it will be done in the function
 * calling this
 */
void Request::insertHeader(const std::string& key, const std::string& value)
{
  Option< std::string > existing = getHeader(key);
  if (isStandardHeader(key) && existing.is_some())
    throw RequestError(400, "Standard Header redefined");

  if (existing.is_none())
  {
    // TODO: Special processing if we get standard headers like Host or
    // Content-Length since they have some extra requirements
    headers_[key] = value;
  }
  else
  {
    std::string delim = (key != "cookie") ? ", " : "; ";
    headers_[key] = existing.unwrap() + delim + value;
  }
}

/*
 * Key should already be lower-case here
 */
bool Request::isStandardHeader(const std::string& key) const
{
  size_t standard_header_count =
      sizeof(STANDARD_HEADERS) / sizeof(STANDARD_HEADERS[0]);

  for (size_t i = 0; i < standard_header_count; ++i)
  {
    if (key == STANDARD_HEADERS[i])
      return (true);
  }

  return (false);
}

void Request::validateTransferEncoding(const std::string& value)
{
  std::istringstream stream(value);
  std::string part;

  while (std::getline(stream, part, ','))
  {
    std::string trimmed = Utils::trimString(part);
    if (trimmed.empty())
      throw RequestError(400, "Empty part in transfer encoding");
    std::for_each(trimmed.begin(), trimmed.end(), Utils::toLower);
    if (trimmed != "chunked")
      throw RequestError(501, "Invalid Transfer-Encoding");
    if (chunked_)
      throw RequestError(400, "Chunked header defined multiple times");
    chunked_ = true;
  }
}

void Request::validateContentLength(const std::string& value)
{
  std::istringstream stream(value);
  long number;

  if (!(stream >> number) || number < 0 || !stream.eof())
    throw RequestError(400, "Invalid Content-Length header");
  content_length_ = Option< long >(number);
}

const location& Request::findMatchingLocationBlock(
    const MLocations& locations,
    const std::string& path) const
{
  MLocations::const_iterator it;

  it = locations.find(path);
  if (it != locations.end())
    return it->second;

  for (it = locations.begin(); it != locations.end(); ++it)
  {
    if (*it->first.rbegin() == '/' &&
        path.substr(0, it->first.length()) == it->first)
    {
      return it->second;
    }
  }

  throw RequestError(404, "No matching location found");
}
