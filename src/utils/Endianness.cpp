#include <sys/types.h>
#include <sstream>
#include <string>

namespace Utils
{
  bool isBigEndian()
  {
    unsigned int num = 1;
    unsigned char* converted = (unsigned char*)&num;

    return (*converted == 0);
  }

  /* u_int32_t ipv4ToBigEndian(u_int8_t octets[4])
  {
    if (isBigEndian())
    {
      return octets[0] << 24 | octets[1] << 16 | octets[2] << 8 | octets[3];
    }
    return octets[3] << 24 | octets[2] << 16 | octets[1] << 8 | octets[0];
  } */
  u_int32_t ipv4ToBigEndian(u_int8_t octets[4])
  {
    u_int32_t result = 0;

    result =
        (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return result;
  }

  /*
   * This expects the input to be in network byte order - i.e. directly coming
   * from the struct sockaddr_in
   */
  std::string ipv4ToString(u_int32_t addr)
  {
    unsigned char* bytes = reinterpret_cast< unsigned char* >(&addr);
    std::ostringstream stream;
    for (int i = 0; i < 4; i++)
    {
      if (i != 0)
        stream << ".";
      stream << static_cast< u_int16_t >(bytes[i]);
    }
    return stream.str();
  }

  u_int16_t u16ToBigEndian(u_int16_t value)
  {
    return (value << 8) | (value >> 8);
  }
}  // namespace Utils
