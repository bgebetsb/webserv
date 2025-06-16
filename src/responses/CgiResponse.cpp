#include "CgiResponse.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include "../Configs/Configs.hpp"
#include "../epoll/PipeFd.hpp"
#include "../exceptions/ConError.hpp"
#include "../exceptions/ExitExc.hpp"
#include "../exceptions/RequestError.hpp"
#include "../requests/Request.hpp"
#include "../utils/Utils.hpp"

CgiResponse::CgiResponse(int client_fd,
                         bool close,
                         const std::string& cgi_path,
                         const CgiVars& cgi_vars)
    : Response(client_fd, 200, close),
      headers_created_(false),
      status_found_(false),
      meta_variables_(NULL),
      cgi_vars_(cgi_vars),
      cgi_path_(cgi_path),
      last_chunk_sent_(false)
{
  meta_variables_ = implementMetaVariables();

  try
  {
    pipe_fd_ = new PipeFd(full_response_, cgi_vars.script_filename, cgi_path,
                          cgi_vars.input_file, this, meta_variables_,
                          cgi_vars.request_method_enum_);
  }
  catch (std::exception& e)
  {
    deleteMetaVariables();
    throw;
  }
}

CgiResponse::~CgiResponse()
{
  deleteMetaVariables();
  if (pipe_fd_)
  {
    PipeFd* converted = reinterpret_cast< PipeFd* >(pipe_fd_);
    converted->unsetResponse();
  }
}

void CgiResponse::deleteMetaVariables(void)
{
  for (size_t i = 0; meta_variables_[i] != NULL; ++i)
  {
    delete[] meta_variables_[i];
  }
  delete[] meta_variables_;
}

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
    std::string rest(full_response_);
    full_response_ = createGenericResponseLines();
    for (mHeader::iterator it = headers_.begin(); it != headers_.end(); ++it)
    {
      if (it->first != "content-length" && it->first != "transfer-encoding")
        full_response_ += it->first + ": " + it->second + "\r\n";
    }
    full_response_ += "Transfer-Encoding: chunked\r\n";
    std::vector< std::string >::iterator it;
    for (it = cookies_.begin(); it != cookies_.end(); ++it)
      full_response_ += "Set-Cookie: " + *it + "\r\n";
    full_response_ += "\r\n";

    if (!rest.empty())
    {
      std::ostringstream ss;
      ss << std::hex << rest.length() << "\r\n";

      full_response_ += ss.str() + rest + "\r\n";
    }
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
  std::for_each(key.begin(), key.end(), Utils::toLower);
  if (key == "status")
  {
    std::stringstream ss(value);
    int code;
    ss >> code;
    if (ss.fail() || code < 100 || code > 599)
    {
      throw RequestError(500, "Invalid Status header from CGI");
    }
    response_code_ = code;
    response_title_ = Utils::trimString(ss.str().substr(ss.tellg()));
    if (code >= 500)
      close_connection_ = true;
  }
  else if (key == "set-cookie")
    cookies_.push_back(value);
  else
  {
    if (headers_.find(key) == headers_.end())
      headers_[key] = value;
    else
      headers_[key] += ", " + value;
  }
}

void CgiResponse::sendResponse(void)
{
  if (!headers_created_)
  {
    processBuffer();
    if (!headers_created_)
    {
      if (!pipe_fd_)
        throw ExitExc();
      return;  // Don't send anything back until we have at least the headers
    }
  }

  if (full_response_.empty())
  {
    if (pipe_fd_)
      return;

    if (last_chunk_sent_)
    {
      complete_ = true;
      return;
    }

    full_response_ += "0\r\n\r\n";
    last_chunk_sent_ = true;
  }
  size_t amount =
      std::min(static_cast< size_t >(CHUNK_SIZE), full_response_.length());
  std::cout << "Amount to send: " << amount << std::endl;
  ssize_t ret = send(client_fd_, full_response_.c_str(), amount, 0);
  std::cout << "After send, full_response_ size: " << full_response_.size()
            << std::endl;
  if (ret == -1)
    throw ConErr("Peer closed connection");
  else if (ret == 0)
    throw ConErr("Send returned 0?!");
  full_response_ = full_response_.substr(ret);
}

char** CgiResponse::implementMetaVariables()
{
  std::vector< std::string > meta_vars;
  meta_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
  meta_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
  meta_vars.push_back("SERVER_SOFTWARE=42 Webserv powered by Genki/1.0");
  meta_vars.push_back("REDIRECT_STATUS=200");
  meta_vars.push_back("SCRIPT_FILENAME=" + cgi_vars_.script_filename);
  meta_vars.push_back("SCRIPT_NAME=" + cgi_vars_.script_name);
  meta_vars.push_back("REQUEST_METHOD=" + cgi_vars_.request_method_str);
  meta_vars.push_back("PATH=/usr/bin/:/bin");
  if (!cgi_vars_.path_info.empty())
    meta_vars.push_back("PATH_INFO=" + cgi_vars_.path_info);
  meta_vars.push_back("REQUEST_URI=" + cgi_vars_.request_uri);
  meta_vars.push_back("QUERY_STRING=" + cgi_vars_.query_string);
  if (cgi_vars_.request_method_enum_ == POST)
  {
    if (!cgi_vars_.content_type.empty())
      meta_vars.push_back("CONTENT_TYPE=" + cgi_vars_.content_type);
    std::stringstream ss;
    ss << "CONTENT_LENGTH=" << cgi_vars_.file_size;
    meta_vars.push_back(ss.str());
  }
  meta_vars.push_back("SERVER_NAME=" + cgi_vars_.server_name);
  meta_vars.push_back("SERVER_PORT=" + cgi_vars_.server_port);
  meta_vars.push_back("REMOTE_ADDR=" + cgi_vars_.remote_addr);
  meta_vars.push_back("DOCUMENT_ROOT=" + cgi_vars_.document_root);

  std::map< std::string, std::string >::iterator it;
  for (it = cgi_vars_.headers.begin(); it != cgi_vars_.headers.end(); ++it)
    meta_vars.push_back(it->first + "=" + it->second);

  char** envp = new char*[meta_vars.size() + 1]();
  try
  {
    for (size_t i = 0; i < meta_vars.size(); ++i)
    {
      envp[i] = new char[meta_vars[i].length() + 1];
      std::memcpy(envp[i], meta_vars[i].c_str(), meta_vars[i].length());
      envp[i][meta_vars[i].length()] = '\0';
    }
  }
  catch (std::exception& e)
  {
    for (size_t i = 0; i < meta_vars.size(); ++i)
    {
      delete[] envp[i];
    }
    delete[] envp;
    throw;
  }
  return envp;
}

void CgiResponse::unsetPipeFd(void)
{
  pipe_fd_ = NULL;
}

bool CgiResponse::getHeadersCreated() const
{
  return headers_created_;
}

bool CgiResponse::isCgiAndEmpty() const
{
  return full_response_.empty() && pipe_fd_;
}

bool CgiResponse::headersSent() const
{
  return headers_created_;
}
