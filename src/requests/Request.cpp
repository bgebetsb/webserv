#include "Request.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <string>
#include "../responses/DirectoryListing.hpp"
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
    method_ = other.method_;
    host_ = other.host_;
    path_ = other.path_;
    headers_.clear();
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
  server_ = &server;
  const Location& location = findMatchingLocationBlock(server.locations, path_);

  processConnectionHeader();

  if (!methodAllowed(location))
    throw RequestError(405, "Method now allowed");

  Redirection redir = location.redirect;
  if (redir.has_been_set)
  {
    std::string location = redir.uri;
    location = Utils::replaceString(location, "$host", host_);
    location = Utils::replaceString(location, "$request_uri", path_);
    response_ = new RedirectResponse(fd_, redir.code, location, closing_);
    status_ = SENDING_RESPONSE;
    return;
  }

  if (location.root.empty())
    throw RequestError(404, "No root directory set for location");

  std::string full_path =
      location.root + '/' + path_.substr(location.location_name.length());
  processFilePath(full_path, location);
}

void Request::processFilePath(const std::string& path, const Location& location)
{
  PathInfos infos = getFileType(path);

  if (!infos.exists)
    throw RequestError(404, "File doesn't exist");
  else if (path[path.length() - 1] == '/' && infos.types != DIRECTORY)
    throw RequestError(404, "Requested a directory but found a file");
  else if (!infos.readable || infos.types == OTHER)
    throw RequestError(403, "File not readable or incorrect type");
  else if (infos.types == REGULAR_FILE)
  {
    if (method_ == GET)
      setResponse(new FileResponse(fd_, path, 200, closing_));
    else if (method_ == DELETE)
    {
      if (unlink(path.c_str()) == 0)
        setResponse(new StaticResponse(fd_, 204, false, ""));
      else
        throw RequestError(403, "Unable to delete file");
    }
  }
  else if (path[path.length() - 1] != '/')
    throw RequestError(404, "Requested a file but found a directory");
  else
  {
    if (method_ == DELETE)
      throw RequestError(403, "Attempted to delete a directory");
    openDirectory(path, location);
  }
}

int Request::openFile(const std::string& path) const
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
  return fd;
}

void Request::openDirectory(const std::string& path, const Location& location)
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
      std::string redir_loc = "http://" + host_ + path_ + *it + '/';
      setResponse(new RedirectResponse(fd_, 301, redir_loc, closing_));
      return;
    }
    else
    {
      setResponse(new FileResponse(fd_, path + *it, 200, closing_));
      return;
    }
  }

  if (!location.DIR_LISTING.first)
    throw RequestError(403, "No index file found and autoindex disabled");
  createDirectoryListing(path);
}

void Request::createDirectoryListing(const std::string& path)
{
  int fd = openFile(path);
  close(fd);

  std::string content = DirectoryListing::createDirectoryListing(path, path_);
  setResponse(new StaticResponse(fd_, 200, closing_, content));
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

bool Request::methodAllowed(const Location& location) const
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

const Location& Request::findMatchingLocationBlock(const MLocations& locations,
                                                   const std::string& path)
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

const Server& Request::getServer() const
{
  if (server_ == NULL)
    throw RequestError(404, "No matching server found");
  return *server_;
}
