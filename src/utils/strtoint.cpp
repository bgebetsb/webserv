#include <sys/types.h>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
#include "../exceptions/Fatal.hpp"
#include "Utils.hpp"

#define MAX_BODY_SIZE 4UL * 1024 * 1024 * 1024  // 4GB

namespace Utils
{

  u_int8_t ipStrToUint8(const std::string& str)
  {
    char* endptr;
    if (str.empty() || str.length() > 3)
      throw Fatal("Invalid Ipv4 Address format");
    errno = 0;
    long value = strtol(str.c_str(), &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > 255 || errno == ERANGE)
      throw Fatal("Invalid Ipv4 Address format");
    return static_cast< u_int8_t >(value);
  }

  u_int16_t ipStrToUint16(const std::string& str)
  {
    char* endptr;
    if (str.empty() || str.length() > 5)
      throw Fatal("Invalid Ipv4 Address format");
    errno = 0;
    long value = strtol(str.c_str(), &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > 65535 || errno == ERANGE)
      throw Fatal("Invalid Ipv4 Address format");
    return static_cast< u_int16_t >(value);
  }

  size_t strToMaxBodySize(const std::string& str)
  {
    if (str.empty())
      throw Fatal("Empty max body size");
    char* endptr = NULL;
    size_t multiplier = 1;
    errno = 0;
    u_int64_t num_part = strtol(str.c_str(), &endptr, 10);
    if (errno == ERANGE || num_part > MAX_BODY_SIZE)
      throw Fatal("Invalid max body size");
    std::string unit_part(endptr);
    if (unit_part.empty() || unit_part == "b" || unit_part == "B")
      return num_part;
    else if (unit_part == "kb" || unit_part == "KB")
      multiplier = 1024;
    else if (unit_part == "mb" || unit_part == "MB")
      multiplier = 1024 * 1024;
    else if (unit_part == "gb" || unit_part == "GB")
      multiplier = 1024 * 1024 * 1024;
    else
      throw Fatal(
          "Invalid body size unit. Allowed: b, B, kb, KB, mb, MB, gb, GB");
    if (num_part > MAX_BODY_SIZE / multiplier)
      throw Fatal("Invalid max body size");
    return num_part * multiplier;
  }

  u_int16_t errStrToUint16(const std::string& token)
  {
    char* endptr;
    errno = 0;
    if (token.empty() || token.length() > 5)
      throw Fatal("Invalid config file format: expected error code");
    long value = strtol(token.c_str(), &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > 65535 || errno == ERANGE)
      throw Fatal("Invalid config file format: expected error code");
    return static_cast< u_int16_t >(value);
  }

  int parseChunkSize(const std::string& str)
  {
    const char* startptr;
    char* endptr;
    if (str.empty())
      throw Fatal("Empty string to convert to int");
    errno = 0;
    startptr = str.c_str();
    long value = strtol(startptr, &endptr, 16);
    if (startptr == endptr || errno == ERANGE)
      throw Fatal("Invalid string to convert to int");
    if (value < 0 || value > INT_MAX)
      throw Fatal("Negative chunk size or greater than INT_MAX");
    if (*endptr != '\0')
    {
      if (*endptr != ';' &&
          ((*endptr != ' ' && *endptr != '\t') || *(endptr + 1) != ';'))
        throw Fatal("Malformed chunk extension");
    }
    return static_cast< int >(value);
  }
}  // namespace Utils
