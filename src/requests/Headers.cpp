#include <algorithm>
#include <sstream>
#include "../exceptions/RequestError.hpp"
#include "../utils/Utils.hpp"
#include "Request.hpp"
#include "parsing/Parsing.hpp"
#include "requests/RequestMethods.hpp"

static bool isStandardHeader(const std::string& key);

void Request::processHeaderLine(const std::string& line)
{
  if (line.empty())
  {
    validateHeaders();
    return processRequest();
  }

  std::pair< string, string > name_value = Parsing::parseFieldLine(line);
  string name = name_value.first;
  string value = name_value.second;

  std::for_each(name.begin(), name.end(), Utils::toLower);

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

  if (key == "cookie")
    Parsing::validateCookies(value);
  if (key == "host")
    headers_[key] = Parsing::parseHost(value).first;
  else if (existing.is_none())
    headers_[key] = value;
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
    if (method_ == GET || method_ == DELETE)
      closing_ = true;
    if (content_length.is_some())
      throw RequestError(400,
                         "Both Content-Length and Transfer-Encoding present");
  }
  else if (content_length.is_some())
  {
    validateContentLength(content_length.unwrap());
    if ((method_ == GET || method_ == DELETE) && content_length_.unwrap() > 0)
      closing_ = true;
  }

  if (host_.empty())
    host_ = host.unwrap();
}

void Request::validateTransferEncoding(const std::string& value)
{
  bool invalid = false;
  std::istringstream stream(value);
  int c;

  while (true)
  {
    std::string transfer_coding = Parsing::get_token(stream);
    if (transfer_coding == "chunked")
    {
      if (chunked_)
        throw RequestError(400, "Chunked header defined multiple times");
      chunked_ = true;
    }
    else
      invalid = true;
    Parsing::skip_ows(stream);
    c = stream.get();
    if (stream.fail())
      break;
    Parsing::skip_ows(stream);
    if (c == ',')
      continue;
    else if (c == ';')
    {
      Parsing::skip_token(stream);
      Parsing::skip_ows(stream);
      Parsing::skip_character(stream, '=');
      Parsing::skip_ows(stream);
      c = stream.get();
      if (stream.fail())
        throw RequestError(400, "Transfer-Encoding header stopped too soon");
      if (c == '\"')
        Parsing::validateQuotedString(stream);
      else
      {
        stream.unget();
        Parsing::skip_token(stream);
      }
      invalid = true;
      c = stream.get();
      if (stream.fail())
        break;
      Parsing::skip_ows(stream);
      if (c == ',')
        continue;
      else
        throw RequestError(400,
                           "Invalid character in Transfer-Encoding header");
    }
    else
      throw RequestError(400, "Invalid character in Transfer-Encoding header");
  }

  if (invalid)
    throw RequestError(501, "Unsupported value in chunked encoding");
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
