#include "configUtils.hpp"
#include <errno.h>
#include <cstdlib>
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
