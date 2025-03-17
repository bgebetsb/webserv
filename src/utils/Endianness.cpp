#include <sys/types.h>

namespace Utils {
  static bool isBigEndian() {
    unsigned int num = 1;
    unsigned char* converted = (unsigned char*)&num;

    return (*converted == 0);
  }

  u_int16_t u16ToBigEndian(u_int16_t value) {
    if (isBigEndian()) {
      return value;
    }
    return value >> 8 | value << 8;
  }

  u_int32_t ipv4ToBigEndian(u_int8_t octets[4]) {
    if (isBigEndian()) {
      return octets[0] << 24 | octets[1] << 16 | octets[2] << 8 | octets[3];
    }
    return octets[3] << 24 | octets[2] << 16 | octets[1] << 8 | octets[0];
  }
}  // namespace Utils
