#include <algorithm>
#include <sstream>
#include "../exceptions/RequestError.hpp"
#include "../utils/Utils.hpp"
#include "Request.hpp"

static bool isStandardHeader(const std::string& key);

void Request::parseHeaderLine(const std::string& line)
{
  std::string charset("!#$%&'*+-.^_`|~");
  typedef std::string::const_iterator iter_type;
  std::string name;
  std::string value;

  if (line.empty())
  {
    validateHeaders();
    return processRequest();
  }

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
static bool isStandardHeader(const std::string& key)
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

void Request::validateHeaders(void)
{
  Option< std::string > host = getHeader("Host");

  if (host.is_none())
    throw RequestError(400, "Missing Host header");

  Option< std::string > transfer_encoding = getHeader("Transfer-Encoding");
  Option< std::string > content_length = getHeader("Content-Length");
  Option< std::string > filename = getHeader("X-Filename");
  if (filename.is_some())
  {
    filename_ = filename.unwrap();
    if (filename_.empty())
      throw RequestError(400, "X-Filename header cannot be empty");
  }
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

void Request::processConnectionHeader(void)
{
  Option< std::string > header = getHeader("Connection");
  if (header.is_none())
    return;

  std::istringstream stream(header.unwrap());
  std::string part;

  while (std::getline(stream, part, ','))
  {
    part = Utils::trimString(part);
    if (part == "close")
      closing_ = true;
    else if (part == "keep-alive")
      closing_ = false;
    // Ignore everything else since that seems to be nginx behavior
  }
}
