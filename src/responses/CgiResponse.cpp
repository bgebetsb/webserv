#include "CgiResponse.hpp"
#include <string>
#include "Configs/Configs.hpp"
#include "epoll/PipeFd.hpp"
#include "exceptions/RequestError.hpp"
#include "utils/Utils.hpp"

// TODO: This shouldn't always be 200, the CGI could set a Status: header which
// would determine the Response code... Only if the header doesn't exist we
// should fallback to 200. We also need a new Response() constructor for that.
CgiResponse::CgiResponse(int client_fd,
                         bool close,
                         const std::string& cgi_path,
                         const std::string& script_path,
                         const std::string& file_path)
    : Response(client_fd, 200, close),
      headers_created_(false),
      status_found_(false)
{
  // TODO: Give it the actual environment we want to pass to the CGI
  extern char** __environ;

  pipe_fd_ = new PipeFd(full_response_, script_path, cgi_path, file_path, this,
                        __environ);
}

CgiResponse::~CgiResponse() {}

void CgiResponse::processBuffer(void)
{
  size_t pos = full_response_.find('\n');
  while (pos != std::string::npos)
  {
    size_t realpos = pos;
    if (realpos > 0 && full_response_[realpos - 1] == '\r')
    {
      --realpos;
    }
    std::string line(full_response_, 0, realpos);
    if (full_response_.size() > pos + 1)
    {
      full_response_ = std::string(full_response_, pos + 1);
    }
    else
    {
      full_response_.clear();
    }
    addHeaderLine(line);
    if (headers_created_)
      break;
    pos = full_response_.find('\n');
  }
}

void CgiResponse::addHeaderLine(const std::string& line)
{
  if (line.empty())
  {
    // TODO: Create response line, add back all headers which haven't been
    // removed before
    headers_created_ = true;
    return;
  }

  size_t pos = line.find(':');
  if (pos == string::npos)
    throw RequestError(500, "Invalid response from CGI");
  std::string key = line.substr(0, pos);
  if (key.empty() || std::isspace(key[0]))
    throw RequestError(500, "Invalid response from CGI");
  std::string value = Utils::trimString(line.substr(pos + 1));
  if (value.empty())
    throw RequestError(500, "Invalid response from CGI");

  // TODO: Special processing for some headers, probably we should also
  // concatinate duplicate headers like we also do for the Request headers
  headers_[key] = value;
}

void CgiResponse::sendResponse(void)
{
  if (!headers_created_)
  {
    processBuffer();
    if (!headers_created_)
      return;  // Don't send anything back until we have at least the headers
  }

  // TODO: Send content from full_buffer
}
