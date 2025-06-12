#include "configUtils.hpp"
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <set>
#include "../exceptions/Fatal.hpp"
#include "../ip/Ipv6Address.hpp"

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

std::set< u_int32_t > Utils::getIpv4Addresses(const std::string& ip_string)
{
  std::set< u_int32_t > ips;
  struct addrinfo hints;
  struct addrinfo *result, *item;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;

  if (getaddrinfo(ip_string.c_str(), NULL, &hints, &result) != 0)
    throw Fatal("getaddrinfo failed for " + ip_string);

  for (item = result; item != NULL; item = item->ai_next)
  {
    struct sockaddr_in* addr = (struct sockaddr_in*)item->ai_addr;
    ips.insert(addr->sin_addr.s_addr);
  }

  freeaddrinfo(result);

  return ips;
}

std::set< Ipv6 > Utils::getIpv6Addresses(const std::string& ip_string)
{
  std::set< Ipv6 > ips;
  struct addrinfo hints;
  struct addrinfo *result, *item;

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_flags = AI_NUMERICHOST;

  if (getaddrinfo(ip_string.c_str(), NULL, &hints, &result) != 0)
    throw Fatal("getaddrinfo failed for " + ip_string);

  for (item = result; item != NULL; item = item->ai_next)
  {
    struct sockaddr_in6* addr = (struct sockaddr_in6*)item->ai_addr;
    Ipv6 ip;
    std::memcpy(ip.ip, &addr->sin6_addr, 16);
    ips.insert(ip);
  }

  freeaddrinfo(result);

  return ips;
}
