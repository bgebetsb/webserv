#include "configUtils.hpp"
#include <cerrno>
#include <cstdlib>
#include <set>
#include "../exceptions/Fatal.hpp"

#define VALID_URI_CHARS                      \
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"               \
  "abcdefghijklmnopqrstuvwxyz"               \
  "0123456789"                               \
  "-._~"        /* Unreserved characters */  \
  ":/?#[]@"     /* Reserved for full URIs */ \
  "!$&'()*+,;=" /* Sub-delimiters */         \
  "%"

bool Utils::isValidUri(const std::string& uri)
{
  if (uri.find_first_not_of(VALID_URI_CHARS) != std::string::npos)
    return false;
  if (uri.empty())
    return false;
  if (uri[0] == '/')
    return true;
  else if (uri.compare(0, 7, "http://") == 0)
    return true;
  else if (uri.compare(0, 8, "https://") == 0)
    return true;
  else
    return false;
}

u_int32_t Utils::ipStrToUint32Max(const std::string& str, u_int32_t max)
{
  char* endptr;
  errno = 0;
  if (str.empty() || str.length() > 10)
    throw Fatal("Numerical input invalid");
  long value = strtol(str.c_str(), &endptr, 10);
  if (*endptr != '\0' || value < 0 || value > max || errno == ERANGE)
    throw Fatal("Numerical input invalid");
  return static_cast< u_int32_t >(value);
}

static bool isValidHostnameLabel(const std::string& label)
{
  if (label.empty() || label.size() > 63)
    return false;

  if (!std::isalnum(label[0]) || !std::isalnum(label[label.size() - 1]))
    return false;

  for (size_t i = 0; i < label.size(); ++i)
  {
    char c = label[i];
    if (!(std::isalnum(c) || c == '-'))
      return false;
  }
  return true;
}

bool Utils::isValidHostname(const std::string& hostname)  // RFC 1035
{
  if (hostname.empty() || hostname.size() > 255)
    return false;

  size_t start = 0;
  size_t end;

  while ((end = hostname.find('.', start)) != std::string::npos)
  {
    std::string label = hostname.substr(start, end - start);
    if (!isValidHostnameLabel(label))
      return false;
    start = end + 1;
  }

  // Letztes Label prüfen
  std::string lastLabel = hostname.substr(start);
  return isValidHostnameLabel(lastLabel);
}

bool Utils::duplicateEntries(const std::vector< std::string >& entries)
{
  typedef std::vector< std::string > container;
  std::set< std::string > unique;

  container::const_iterator it;
  for (it = entries.begin(); it != entries.end(); ++it)
  {
    if (!unique.insert(*it).second)
      return (true);
  }

  return (false);
}
