#include "Request.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <string>
#include "../responses/DirectoryListing.hpp"
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
#include "../utils/Utils.hpp"
#include "PathValidation/FileTypes.hpp"
#include "PathValidation/PathInfos.hpp"
#include "PathValidation/PathValidation.hpp"
#include "RequestStatus.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/RequestMethods.hpp"
#include "responses/FileResponse.hpp"

std::set< std::string > Request::current_upload_files_;

Request::Request(const int fd, const std::vector< Server >& servers)
    : fd_(fd),
      server_(NULL),
      status_(READING_START_LINE),
      chunked_(false),
      content_length_(Option< long >()),
      closing_(false),
      servers_(servers),
      total_header_size_(0),
      response_(NULL),
      is_cgi_(false),
      file_existed_(false)
{}

Request::Request(const Request& other)
    : fd_(other.fd_),
      server_(other.server_),
      status_(other.status_),
      chunked_(other.chunked_),
      content_length_(other.content_length_),
      closing_(other.closing_),
      servers_(other.servers_),
      total_header_size_(other.total_header_size_),
      response_(other.response_),
      file_existed_(other.file_existed_)
{}

Request& Request::operator=(const Request& other)
{
  if (this != &other)
  {
    fd_ = other.fd_;
    server_ = other.server_;
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
  if (upload_file_.is_open())
  {
    upload_file_.close();
    if (!absolute_path_.empty())
    {
      std::remove(absolute_path_.c_str());
    }
  }
  else if (is_cgi_)
  {
    PathInfos infos = getFileType(absolute_path_);
    if (infos.exists && infos.types == REGULAR_FILE)
      std::remove(absolute_path_.c_str());
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

const std::string& Request::getStartLine() const
{
  return startline_;
}

const std::string& Request::getHost() const
{
  return host_;
}

u_int16_t Request::getResponseCode() const
{
  if (!response_)
    throw;  // The function should only be called when the response has already
            // been sent
  return response_->getResponseCode();
}

void Request::processRequest(void)
{
  const Server& server = getServer(host_);
  server_ = &server;
  const Location& location = findMatchingLocationBlock(server.locations, path_);

  processConnectionHeader();

  if (method_ == INVALID)
    throw RequestError(501, "Method not recognized");
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
  if (location.max_body_size.second)
    max_body_size_ = location.max_body_size.first;
  else
    max_body_size_ = 1024 * 1024;  // Default max body size
  if (isFileUpload(location))
    return (setupFileUpload());
  else if (is_cgi_ == true)
  {
    return (setupCgi());
  }
  std::string full_path = location.root + path_;
  processFilePath(full_path, location);
}

void Request::processFilePath(const std::string& path, const Location& location)
{
  PathInfos infos = getFileType(path);
  if (current_upload_files_.find(path) != current_upload_files_.end())
    throw RequestError(409, "Conflict: File being uploaded");
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
      if (std::remove(path.c_str()) == 0)
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

bool Request::isChunked() const
{
  return chunked_;
}

long Request::getContentLength() const
{
  if (content_length_.is_none())
    throw RequestError(400, "Content-Length header not set");
  return content_length_.unwrap();
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

/*
  This function shall check in case of a directory request if the cgi extension
  is found in the index files
  If yes it shall set CGI to true and return false
  If no it shall return true -> Random filename is going to be generated
*/
bool Request::CgiOrUpload(const Location& loc)
{
  for (VDefaultFiles::const_iterator it = loc.default_files.begin();
       it != loc.default_files.end(); ++it)
  {
    std::string file_extension = it->substr(it->find_last_of("."));
    if (loc.cgi_extensions.find(file_extension) != loc.cgi_extensions.end())
    {
      is_cgi_ = true;
      if (file_extension == ".php")
        cgi_extension_ = PHP;
      else if (file_extension == ".py")
        cgi_extension_ = PYTHON;
      cgi_skript_path_ = loc.root + "/" + *it;
      return false;  // CGI found, no upload
    }
  }
  is_cgi_ = false;
  while (1)
  {
    filename_ = generateRandomFilename();
    absolute_path_ = loc.root + "/" + loc.upload_dir + "/" + filename_;
    PathInfos infos = getFileType(absolute_path_);
    if (!infos.exists)
    {
      current_upload_files_.insert(absolute_path_);
      break;  // Found a random filename that does not exist
    }
    // else continue to generate a new random filename
  }
  return true;  // No CGI found, upload is possible
}

/*
  This function is to check if the request is a file upload or if it is a cgi
  since this gets treated differently
  If no it shall return true -> Random filename is going to be generated
*/
bool Request::isFileUpload(const Location& loc)
{
  filename_ = path_.substr(loc.location_name.length());
  if (method_ != POST)
    return false;  // Not a POST request, no file upload
  if (status_ == READING_BODY)
    throw;
  // ── ◼︎ check for upload dir ───────────────────────────────────────────
  if (loc.upload_dir.empty())
    throw RequestError(403, "No upload dir set");
  else
  {
    PathInfos infos = getFileType(loc.root + "/" + loc.upload_dir);
    if (!infos.exists || infos.types != DIRECTORY || !infos.writable)
    {
      throw RequestError(
          500, "Upload dir not found or not a directory or not writable");
      return false;
    }
  }

  // ── ◼︎ check for empty filename & CGI ─────────────────────────────────────
  if (filename_.empty())
    return CgiOrUpload(loc);

  // ── ◼︎ check for cgi extension in filename ─────────────────────────────
  std::string file_extension = filename_.substr(filename_.find_last_of("."));
  if (!file_extension.empty() &&
      loc.cgi_extensions.find(file_extension) != loc.cgi_extensions.end())
  {
    is_cgi_ = true;
    if (file_extension == ".php")
      cgi_extension_ = PHP;
    else if (file_extension == ".py")
      cgi_extension_ = PYTHON;
    cgi_skript_path_ = loc.root + "/" + filename_;
    return false;
  }

  // ── ◼︎ if no cgi, check for slash -> forbidden ───────────────────────
  if (filename_.find_first_of("/") != std::string::npos)
    throw RequestError(400, "Invalid filename_");

  // ── ◼︎ check for file      ──────────────────────────────────────────────
  absolute_path_ = loc.root + "/" + loc.upload_dir + "/" + filename_;
  PathInfos infos = getFileType(absolute_path_);
  if (!infos.exists)
    return true;
  else if (infos.exists && infos.types == REGULAR_FILE && infos.writable)
  {
    file_existed_ = true;
    return true;
  }
  else
  {
    throw RequestError(403, "File not accesible or incorrect type");
    return false;
  }
}

void Request::setupFileUpload()
{
  std::cout << "Setting up file upload for: " << absolute_path_ << std::endl;
  if (current_upload_files_.insert(absolute_path_).second)
  {
    std::cout << "Setting up file upload for: " << absolute_path_ << std::endl;
    errno = 0;  // Reset errno before opening the file
    upload_file_.open(absolute_path_.c_str(), std::ios::out | std::ios::binary);
    if (errno == ENAMETOOLONG)
    {
      std::cout << "ERRNO: " << errno << std::endl;
      throw RequestError(400, "Filename too long");
    }
    total_written_bytes_ = 0;
    status_ = READING_BODY;
    return;
  }
  else
  {
    throw RequestError(409, "File already exists");
  }
}

void Request::setupCgi()
{
  PathInfos infos = getFileType(cgi_skript_path_);
  if (!infos.exists || infos.types != REGULAR_FILE)
    throw RequestError(404, "CGI Skript not found");
  // PathInfos
  while (1)
  {
    filename_ = generateRandomFilename();
    absolute_path_ = "/tmp/" + filename_;
    PathInfos infos = getFileType(absolute_path_);
    if (!infos.exists)
      break;  // Found a random filename that does not exist
    // else continue to generate a new random filename
  }
  std::cout << "Setting up CGI for: " << absolute_path_ << std::endl;
  current_upload_files_.insert(absolute_path_);
  upload_file_.open(absolute_path_.c_str(), std::ios::out | std::ios::binary);
  total_written_bytes_ = 0;
  status_ = READING_BODY;
}

const Server& Request::getServer() const
{
  if (server_ == NULL)
    throw RequestError(404, "No matching server found");
  return *server_;
}

long Request::getMaxBodySize() const
{
  return max_body_size_;
}
