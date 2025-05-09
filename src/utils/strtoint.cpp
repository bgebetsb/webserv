#include <cstdlib>
#include <stdexcept>
#include "Utils.hpp"

namespace Utils
{

  u_int8_t strtouint8(const std::string& str)
  {
    char* endptr;
    if (str.empty() || str.length() > 3)
      throw std::runtime_error("Invalid Ipv4 Address format");
    long value = strtol(str.c_str(), &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > 255)
      throw std::runtime_error("Invalid Ipv4 Address format");
    return static_cast< u_int8_t >(value);
  }

  u_int16_t strtouint16(const std::string& str)
  {
    char* endptr;
    if (str.empty() || str.length() > 5)
      throw std::runtime_error("Invalid Ipv4 Address format");
    long value = strtol(str.c_str(), &endptr, 10);
    if (*endptr != '\0' || value < 0 || value > 65535)
      throw std::runtime_error("Invalid Ipv4 Address format");
    return static_cast< u_int16_t >(value);
  }

}  // namespace Utils
