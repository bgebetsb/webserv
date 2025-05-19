#include "Request.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstddef>
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
      // In any other case it should not go here
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

void Request::processRequest(void)
{
  const Server& server = getServer(host_);
  const location& location = findMatchingLocationBlock(server.locations, path_);

  processConnectionHeader();

  if (!methodAllowed(location))
    throw RequestError(405, "Method now allowed");

  redirection redir = location.redirect;
  if (redir.has_been_set)
  {
    std::string location = redir.uri;
    location = Utils::replaceString(location, "$host", host_);
    location = Utils::replaceString(location, "$request_uri", path_);
    response_ = new RedirectResponse(fd_, redir.code, location, closing_);
    status_ = SENDING_RESPONSE;
    return;
  }
  if (isValidPostRequest(location))
  {
    ;  // processPostRequest();
  }
  if (location.root.empty())
    throw RequestError(404, "No root directory set for location");

  std::string full_path = location.root + path_;
  processFilePath(full_path, location);
}

void Request::processFilePath(const std::string& path, const location& location)
{
  PathInfos infos = getFileType(path);

  if (!infos.exists)
    throw RequestError(404, "File doesn't exist");
  else if (!infos.readable || infos.types == OTHER)
    throw RequestError(403, "File not readable or incorrect type");
  else if (infos.types == REGULAR_FILE)
  {
    openFile(path, infos.size);
    // TODO: CGI
    // if (method_ == POST && location.cgi_extensions)
    // {
    //   postRequestCGI();
    //   return;
    // }
  }
  else
    openDirectory(path, location);
}

void Request::openFile(const std::string& path, off_t size)
{
  int fd = open(path.c_str(), O_RDONLY | O_NOFOLLOW);
  if (fd == -1)
  {
    if (errno == ELOOP)
      throw RequestError(403, "Requested file is a symlink");
    else if (errno == ENFILE || errno == EMFILE)
      throw RequestError(503, "Server ran out of fds");
    throw RequestError(500, "Open failed for unknown reason");
  }
  else
  {
    response_ = new FileResponse(fd_, fd, size, closing_);
    status_ = SENDING_RESPONSE;
  }
}

void Request::openDirectory(const std::string& path, const location& location)
{
  VDefaultFiles::const_iterator it;
  const VDefaultFiles& files = location.default_files;

  for (it = files.begin(); it != files.end(); ++it)
  {
    PathInfos infos = getFileType(path + *it);
    if (!infos.exists)
      continue;
    if (!infos.readable || infos.types == OTHER)
      throw RequestError(403, "File not readable or incorrect type");
    else if (infos.types == DIRECTORY)
    {
      // TODO: Respond with 301 permanent redirect to the directory, just in
      // case we have a directory called index.html (or similar) lol
      throw RequestError(501, "index file pointing to directory");
    }
    else
      return openFile(path + *it, infos.size);
  }

  if (!location.DIR_LISTING.first)
    throw RequestError(403, "No index file found and autoindex disabled");
  throw RequestError(501, "Directory listings not implemented yet");
}

bool Request::closingConnection() const
{
  return closing_;
}

// TODO: Maybe remove this and just use the setResponse function from outside
void Request::timeout()
{
  response_ = new StaticResponse(fd_, 408, true);
  status_ = SENDING_RESPONSE;
}

void Request::setResponse(Response* response)
{
  response_ = response;
  status_ = SENDING_RESPONSE;
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

// bool Request::isValidPostRequest(const location& loc) const
// {
//   std::string file_extension = path_.substr(path_.find_last_of(".") + 1);
//   if (method_ != POST)
//     return false;
//   if (headers_.find("Content-Type") == headers_.end())
//     return false;
//   if (headers_.find("Content-Length") == headers_.end())
//     return false;
// TODO: enum erstellen für die verschiedenen POST typen
//   chunked existiert schon.