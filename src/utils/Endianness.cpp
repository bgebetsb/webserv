#include <netinet/in.h>
#include <sys/types.h>
#include <sstream>
#include <string>
#include <vector>

namespace Utils
{
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

  std::string ipv6ToString(struct in6_addr& address)
  {
    std::vector< std::string > parts;
    for (unsigned int i = 0; i < 8; i++)
    {
      std::ostringstream stream;
      stream << ntohs(address.s6_addr16[i]);
      parts.push_back(stream.str());
    }

    std::string::size_type longest_pos = 0;
    unsigned int longest_amount = 0;
    for (unsigned int i = 0; i < 8; i++)
    {
      if (parts[i] == "0")
      {
        unsigned int amount = 1;
        for (unsigned int j = i + 1; j < 8; j++)
        {
          if (parts[j] == "0")
            amount++;
          else
            break;
        }
        if (amount > longest_amount)
        {
          longest_amount = amount;
          longest_pos = i;
        }
      }
    }
    if (longest_amount > 1)
    {
      parts.erase(parts.begin() + longest_pos,
                  parts.begin() + longest_pos + longest_amount);
      parts.insert(parts.begin() + longest_pos, ":");
    }

    std::ostringstream stream;
    stream << "[";
    if (parts.size() == 1)
      stream << ":";
    for (unsigned int i = 0; i < parts.size(); ++i)
    {
      if (i != 0)
        stream << ":";
      stream << parts[i];
    }
    stream << "]";

    return stream.str();
  }
}  // namespace Utils
