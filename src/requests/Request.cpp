#include "Request.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>
#include "../Server.hpp"
#include "../exceptions/ConError.hpp"
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
#include "../utils/Utils.hpp"
#include "Configs/Configs.hpp"
#include "Option.hpp"
#include "PathValidation/FileTypes.hpp"
#include "PathValidation/PathInfos.hpp"
#include "PathValidation/PathValidation.hpp"
#include "RequestStatus.hpp"
#include "responses/FileResponse.hpp"
#include "responses/Response.hpp"

Request::Request(const int fd, const std::vector< Server >& servers)
    : fd_(fd),
      status_(READING_START_LINE),
      closing_(false),
      servers_(servers),
      total_header_size_(0),
      response_(NULL)
{}

Request::Request(const Request& other)
    : fd_(other.fd_),
      status_(other.status_),
      closing_(other.closing_),
      servers_(other.servers_),
      total_header_size_(other.total_header_size_),
      response_(other.response_)
{}

Request::~Request()
{
  delete response_;
}

void Request::addHeaderLine(const std::string& line)
{
  size_t pos = line.find('\r');
  if (pos != std::string::npos)
  {
    response_ = new StaticResponse(fd_, 400);
    status_ = SENDING_RESPONSE;
    return;
  }

  total_header_size_ += line.size();
  if (total_header_size_ > 32768)
  {
    response_ = new StaticResponse(fd_, 400);
    status_ = SENDING_RESPONSE;
    return;
  }

  try
  {
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
  catch (ConErr& e)
  {
    std::cerr << e.what() << "\n";
    response_ = new StaticResponse(fd_, 400);
    status_ = SENDING_RESPONSE;
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
  {
    throw ConErr("Header does not contain a ':' character");
  }
  name = line.substr(0, pos);
  value = Utils::trimString(line.substr(pos + 1));

  std::for_each(name.begin(), name.end(), Utils::toLower);

  for (iter_type it = name.begin(); it < name.end(); ++it)
  {
    char c = *it;

    if (charset.find(c) == std::string::npos && !std::isalpha(c) &&
        !std::isdigit(c))
    {
      throw ConErr("Invalid character");
    }
  }

  // TODO: Special processing if we get the `Host` header, since it has some
  // extra

  // std::cout << "Key: " << name << ", Value: " << value << "\n";
  headers_[name] = value;
}

void Request::processHeaders(void)
{
  Option< std::string > host = getHeader("Host");

  if (host.is_none())
  {
    response_ = new StaticResponse(fd_, 400);
    status_ = SENDING_RESPONSE;
    return;
  }

  const Server& server = getServer(host.unwrap());

  MLocations locations = server.getServerConfigs().locations;
  MLocations::const_iterator l_it = locations.find(path_);

  PathInfos infos;

  if (l_it == locations.end() || l_it->second.root.empty())
  {
    response_ = new StaticResponse(fd_, 404);
    status_ = SENDING_RESPONSE;
    return;
  }

  MRedirects::const_iterator redirect = l_it->second.redirects.find(path_);
  if (redirect != l_it->second.redirects.end())
  {
    response_ = new RedirectResponse(fd_, redirect->second);
    status_ = SENDING_RESPONSE;
    return;
  }

  if (l_it->second.root.empty())
  {
    response_ = new StaticResponse(fd_, 404);
    status_ = SENDING_RESPONSE;
    return;
  }

  // TODO: Actually search the correct location
  std::string full_path = locations[0].root + path_;

  infos = getFileType(full_path);

  if (!infos.exists)
  {
    response_ = new StaticResponse(fd_, 404);
  }
  else if (!infos.readable || infos.types == OTHER)
  {
    response_ = new StaticResponse(fd_, 403);
  }
  else
  {
    int fd = open(full_path.c_str(), O_RDONLY | O_NOFOLLOW);
    if (fd == -1)
    {
      response_ = new StaticResponse(fd_, 500);
    }
    else
    {
      response_ = new FileResponse(fd_, fd, infos.size);
    }
  }
  status_ = SENDING_RESPONSE;
}

bool Request::closingConnection() const
{
  return closing_;
}

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
    const serv_config& config = server->getServerConfigs();
    if (config.server_names.find(host) != config.server_names.end())
      return *server;
  }

  return servers_.front();
}
