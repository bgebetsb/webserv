#include "Request.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include "../Configs/Configs.hpp"
#include "../epoll/EpollData.hpp"
#include "../exceptions/ConError.hpp"
#include "../exceptions/ExitExc.hpp"
#include "../exceptions/RequestError.hpp"
#include "../requests/CgiVars.hpp"
#include "../responses/CgiResponse.hpp"
#include "../responses/DirectoryListing.hpp"
#include "../responses/FileResponse.hpp"
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
#include "../utils/Utils.hpp"
#include "PathValidation/FileTypes.hpp"
#include "PathValidation/PathInfos.hpp"
#include "PathValidation/PathValidation.hpp"
#include "RequestMethods.hpp"
#include "RequestStatus.hpp"

std::set< std::string > Request::current_upload_files_;

Request::Request(const int fd,
                 const std::vector< Server >& servers,
                 const std::string& client_ip)
    : fd_(fd),
      client_ip_(client_ip),
      server_(NULL),
      status_(READING_START_LINE),
      method_(INVALID),
      chunked_(false),
      content_length_(Option< long >()),
      closing_(false),
      servers_(servers),
      total_header_size_(0),
      response_(NULL),
      is_cgi_(false),
      file_existed_(false),
      total_written_bytes_(0)
{}

Request::Request(const Request& other)
    : fd_(other.fd_),
      client_ip_(other.client_ip_),
      server_(other.server_),
      status_(other.status_),
      method_(other.method_),
      host_(other.host_),
      path_(other.path_),
      headers_(other.headers_),
      chunked_(other.chunked_),
      content_length_(other.content_length_),
      closing_(other.closing_),
      servers_(other.servers_),
      total_header_size_(other.total_header_size_),
      response_(other.response_),
      is_cgi_(other.is_cgi_),
      filename_(other.filename_),
      file_existed_(other.file_existed_),
      total_written_bytes_(other.total_written_bytes_)
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
    headers_ = other.headers_;
    chunked_ = other.chunked_;
    content_length_ = other.content_length_;
    closing_ = other.closing_;
    total_header_size_ = other.total_header_size_;
    if (response_)
    {
      delete response_;
    }
    response_ = other.response_;
    is_cgi_ = other.is_cgi_;
    file_existed_ = other.file_existed_;
    total_written_bytes_ = other.total_written_bytes_;
  }
  filename_ = other.filename_;

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
  if (current_upload_files_.find(absolute_path_) != current_upload_files_.end())
  {
    current_upload_files_.erase(absolute_path_);
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
      return processHeaderLine(line);
    default:
      // In any other case it should not go here
      throw;
  }
}

void Request::sendResponse()
{
  try
  {
    response_->sendResponse();
    if (response_->isComplete())
    {
      status_ = COMPLETED;
      closing_ = response_->getClosing();
    }
    else if (response_->isCgiAndEmpty())
    {
      EpollData& ep_data = getEpollData();
      EpollFd* fd = ep_data.fds[fd_];
      struct epoll_event* event = fd->getEvent();
      event->events = 0;
      if (epoll_ctl(ep_data.fd, EPOLL_CTL_MOD, fd_, event) == -1)
        throw ConErr("Failed to modify epoll event");
    }
  }
  catch (RequestError& e)
  {
    std::cerr << e.what() << std::endl;
    delete response_;
    response_ = new StaticResponse(fd_, 500, true);
  }
  catch (ExitExc& e)
  {
    std::cerr << e.what() << std::endl;
    delete response_;
    response_ = new StaticResponse(fd_, 500, true);
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
    throw RequestError(405, "Method not allowed");

  Redirection redir = location.redirect;
  if (redir.has_been_set)
  {
    std::string location = redir.uri;
    std::string host = host_;
    if (port_ != "80")
      host += ":" + port_;
    location = Utils::replaceString(location, "$host", host);
    location = Utils::replaceString(location, "$request_uri", uri_);
    response_ = new RedirectResponse(fd_, redir.code, location, closing_);
    status_ = SENDING_RESPONSE;
    return;
  }

  if (location.root.empty())
    throw RequestError(404, "No root directory set for location");
  splitPathInfo(location);
  if (path_[path_.length() - 1] != '/')
  {
    PathInfos infos = getFileType(location.root + "/" + path_);
    if (infos.exists && infos.types == DIRECTORY)
    {
      string url = "http://" + host_;
      if (port_ != "80")
        url += ":" + port_;
      url += path_ + "/";
      response_ = new RedirectResponse(fd_, 301, url, closing_);
      status_ = SENDING_RESPONSE;
      return;
    }
  }
  if (location.max_body_size.second)
    max_body_size_ = location.max_body_size.first;
  else
    max_body_size_ = 1024 * 1024;  // Default max body size
  checkForCgi(location);
  bool is_upload = isFileUpload(location);
  if (is_cgi_)
  {
    PathInfos infos = getFileType(cgi_script_filename_);
    if (!infos.exists || infos.types != REGULAR_FILE)
      throw RequestError(404, "CGI Skript not found");
    // PathInfos
  }
  if (is_upload && is_cgi_ == false)
    return (setupFileUpload());
  else if (is_upload && is_cgi_ == true)
    return (setupCgi());
  else if (is_cgi_)
  {
    CgiVars cgi_vars = createCgiVars();
    std::string cgi_bin_path;
    cgi_extension_ == PHP
        ? cgi_bin_path = Configuration::getInstance().getPhpPath()
        : cgi_bin_path = Configuration::getInstance().getPythonPath();
    std::string cookies;
    if (getHeader("cookie").is_some())
      cookies = getHeader("cookie").unwrap();
    else
      cookies = "";
    response_ = new CgiResponse(fd_, closing_, cgi_bin_path, cgi_vars);
    status_ = SENDING_RESPONSE;
    return;
  }
  std::string full_path =
      location.root + path_.substr(location.location_name.find_last_of('/'));
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
      std::string host = host_;
      if (port_ != "80")
        host += ":" + port_;
      std::string redir_loc = "http://" + host + path_ + *it + '/';
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

void Request::setResponse(Response* response)
{
  if (response_)
  {
    delete response_;
  }
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
    throw RequestError(411, "Content-Length header not set");
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

void Request::checkForCgi(const Location& loc)
{
  is_cgi_ = false;
  if (!filename_.empty())
    return;

  std::string skriptname = path_.substr(loc.location_name.find_last_of('/'));
  if (skriptname.empty())
    skriptname = "/";
  if (skriptname[skriptname.length() - 1] == '/')
  {
    for (VDefaultFiles::const_iterator it = loc.default_files.begin();
         it != loc.default_files.end(); ++it)
    {
      string optional_slash;
      if (loc.root[loc.root.length() - 1] != '/' && skriptname[0] != '/')
        optional_slash = "/";
      string filename = loc.root + optional_slash + skriptname + *it;
      PathInfos infos = getFileType(filename);  // Check if default file exists
      if (infos.exists == true && infos.types == REGULAR_FILE &&
          infos.readable == true)
      {
        std::string file_extension = it->substr(it->find_last_of("."));
        if (loc.cgi_extensions.find(file_extension) != loc.cgi_extensions.end())
        {
          is_cgi_ = true;
          if (file_extension == ".php")
            cgi_extension_ = PHP;
          else if (file_extension == ".py")
            cgi_extension_ = PYTHON;
          cgi_script_filename_ = filename;
          cgi_script_filename_ =
              Utils::replaceString(cgi_script_filename_, "//", "/");
          if (skriptname[0] == '/' ||
              loc.location_name[loc.location_name.length() - 1] == '/')
            optional_slash = "";
          cgi_script_name_ =
              loc.location_name + optional_slash + skriptname + *it;
          cgi_script_name_ = Utils::replaceString(cgi_script_name_, "//", "/");
          document_root_ = loc.root;
        }
        return;  // Found a default file (either cgi or not cgi)
      }
    }
  }
  else
  {
    std::string::size_type pos = skriptname.find_last_of(".");
    if (pos == std::string::npos)
      return;
    std::string file_extension = skriptname.substr(pos);
    if (!file_extension.empty() &&
        loc.cgi_extensions.find(file_extension) != loc.cgi_extensions.end())
    {
      PathInfos infos =
          getFileType(loc.root + "/" + skriptname);  // Check if skript exists
      if (!infos.exists || infos.types != REGULAR_FILE || !infos.readable)
        throw RequestError(404, "CGI Skript not found");
      is_cgi_ = true;
      if (file_extension == ".php")
        cgi_extension_ = PHP;
      else if (file_extension == ".py")
        cgi_extension_ = PYTHON;
      cgi_script_filename_ = loc.root + "/" + skriptname;
      cgi_script_name_ = loc.location_name;
      if (loc.location_name[loc.location_name.length() - 1] != '/' &&
          skriptname[0] != '/')
        cgi_script_name_ += '/';
      cgi_script_name_ += skriptname;
      cgi_script_filename_ =
          Utils::replaceString(cgi_script_filename_, "//", "/");
      cgi_script_name_ = Utils::replaceString(cgi_script_name_, "//", "/");
      document_root_ = loc.root;
    }
  }
}

/*
  This function is to check if the request is a file upload or if it is a cgi
  since this gets treated differently
  If no it shall return true -> Random  is going to be generated
*/
bool Request::isFileUpload(const Location& loc)
{
  upload_dir_ = loc.upload_dir;
  if (method_ != POST)
    return false;  // Not a POST request, no file upload
  if (is_cgi_ == false && path_[path_.length() - 1] != '/')
    throw RequestError(405, "File upload only allowed on directories");
  if (status_ == READING_BODY)
    throw;

  // ── ◼︎ check for upload dir
  // ──────────────────────────────────────
  if (loc.upload_dir.empty() && is_cgi_ == false)
    throw RequestError(403, "No upload dir set");
  else if (is_cgi_ == false)
  {
    PathInfos infos = getFileType(loc.root + "/" + loc.upload_dir);
    if (!infos.exists || infos.types != DIRECTORY || !infos.writable)
    {
      throw RequestError(
          500, "Upload dir not found or not a directory or not writable");
      return false;
    }
  }

  // ── ◼︎ check for empty filename & CGI
  // ───────────────────────────────
  if (filename_.empty() && is_cgi_ == false)
  {
    // ── ◼︎ generate random filename
    while (1)
    {
      filename_ = generateRandomFilename();
      absolute_path_ = loc.root + "/" + loc.upload_dir + "/" + filename_;
      PathInfos infos = getFileType(absolute_path_);
      if (!infos.exists)

        break;  // Found a random filename that does not exist

      // else continue to generate a new random filename
    }
  }
  else if (is_cgi_ == false)
  {
    std::string::size_type pos_dot = filename_.find_last_of(("."));
    if (pos_dot != std::string::npos)
    {
      std::string file_extension = filename_.substr(pos_dot);
      if (file_extension == ".php" || file_extension == ".py")
        throw RequestError(403, "File upload not allowed for CGI skripts");
    }
  }

  // ── ◼︎ if no cgi, check for slash -> forbidden
  // ───────────────────
  if (is_cgi_ == false && filename_.find_first_of("/") != std::string::npos)
    throw RequestError(400, "Invalid filename_");
  if (is_cgi_ == true)
    return true;  // CGI requests always have a filename
  // ── ◼︎ check for file
  // ────────────────────────────────────────────

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
  if (current_upload_files_.insert(absolute_path_).second)
  {
    errno = 0;  // Reset errno before opening the file
    upload_file_.open(absolute_path_.c_str(),
                      O_CREAT | O_TRUNC | O_CLOEXEC | O_WRONLY);
    if (errno == ENAMETOOLONG)
    {
      throw RequestError(400, "Filename too long");
    }
    total_written_bytes_ = 0;
    status_ = READING_BODY;
    return;
  }
  else
  {
    throw RequestError(409, "File current in upload");
  }
}

void Request::setupCgi()
{
  while (1)
  {
    filename_ = generateRandomFilename();
    absolute_path_ = "/tmp/" + filename_;
    PathInfos infos = getFileType(absolute_path_);
    if (!infos.exists)
      break;  // Found a random filename that does not exist
    // else continue to generate a new random filename
  }
  current_upload_files_.insert(absolute_path_);
  upload_file_.open(absolute_path_.c_str(),
                    O_CREAT | O_TRUNC | O_CLOEXEC | O_WRONLY);
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

CgiVars Request::createCgiVars(void) const
{
  CgiVars cgi_vars;
  mHeader::const_iterator it;

  cgi_vars.input_file = absolute_path_;
  cgi_vars.file_size = total_written_bytes_;
  cgi_vars.request_method_enum_ = method_;
  cgi_vars.request_method_str = methodToString(method_);
  cgi_vars.request_uri = uri_;
  cgi_vars.server_name = host_;
  cgi_vars.server_port = port_;
  cgi_vars.remote_addr = client_ip_;
  cgi_vars.path_info = path_info_;
  cgi_vars.script_filename = cgi_script_filename_;
  cgi_vars.script_name = cgi_script_name_;
  cgi_vars.query_string = query_string_;
  cgi_vars.document_root = document_root_;

  for (it = headers_.begin(); it != headers_.end(); ++it)
  {
    if (it->first == "content-type")
      cgi_vars.content_type = it->second;
    else if (it->first != "content-length" &&
             it->first != "transfer-encoding" && it->first != "connection")
    {
      std::string key = it->first;
      std::for_each(key.begin(), key.end(), Utils::toUpperWithUnderscores);
      cgi_vars.headers["HTTP_" + key] = it->second;
    }
  }

  return cgi_vars;
}

/*
 * This might look a bit over-engineered, but its actually needed in case
 * someone is crazy enough to create a directory which ends with one of the CGI
 * extensions
 */
void Request::splitPathInfo(const Location& location)
{
  std::set< std::string::size_type > positions;

  std::string::size_type pos = path_.find(".php/");
  while (pos != std::string::npos)
  {
    std::string::size_type slash_pos = pos + 4;
    positions.insert(slash_pos);
    pos = path_.find(".php/", slash_pos + 1);
  }

  pos = path_.find(".py/");
  while (pos != std::string::npos)
  {
    std::string::size_type slash_pos = pos + 3;
    positions.insert(slash_pos);
    pos = path_.find(".php/", slash_pos + 1);
  }

  std::set< std::string::size_type >::iterator it;
  for (it = positions.begin(); it != positions.end(); ++it)
  {
    std::string path =
        location.root +
        path_.substr(0, *it).substr(location.location_name.find_last_of('/'));
    PathInfos infos = getFileType(path);
    if (infos.exists && infos.types == REGULAR_FILE)
    {
      path_info_ = path_.substr(*it);
      path_ = path_.substr(0, *it);
      return;
    }
  }
}
