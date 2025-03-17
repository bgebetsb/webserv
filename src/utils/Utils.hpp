#pragma once

#include <sys/types.h>

namespace Utils {
  u_int16_t u16ToBigEndian(u_int16_t value);
  u_int32_t ipv4ToBigEndian(u_int8_t octets[4]);
}  // namespace Utils
