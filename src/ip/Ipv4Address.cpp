#include "Ipv4Address.hpp"
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include "../utils/Utils.hpp"
#include "IpAddress.hpp"

static u_int8_t strtouint8(std::string& str)
{
  char* endptr;
  if (str.empty() || str.length() > 3)
    throw std::runtime_error("Invalid Ip Address format");
  long value = strtol(str.c_str(), &endptr, 10);
  if (*endptr != '\0' || value < 0 || value > 255)
    throw std::runtime_error("Invalid Ip Address format");
  return static_cast< u_int8_t >(value);
}

static u_int16_t strtouint16(std::string& str)
{
  char* endptr;
  if (str.empty() || str.length() > 5)
    throw std::runtime_error("Invalid Ip Address format");
  long value = strtol(str.c_str(), &endptr, 10);
  if (*endptr != '\0' || value < 0 || value > 65535)
    throw std::runtime_error("Invalid Ip Address format");
  return static_cast< u_int16_t >(value);
}

std::ostream& operator<<(std::ostream& os, const Ipv4Address& addr)
{
  u_int8_t octets[4];
  if (Utils::isBigEndian())
  {
    octets[0] = (addr.getIp() >> 24) & 0xFF;
    octets[1] = (addr.getIp() >> 16) & 0xFF;
    octets[2] = (addr.getIp() >> 8) & 0xFF;
    octets[3] = addr.getIp() & 0xFF;
  } else
  {
    octets[0] = addr.getIp() & 0xFF;
    octets[1] = (addr.getIp() >> 8) & 0xFF;
    octets[2] = (addr.getIp() >> 16) & 0xFF;
    octets[3] = (addr.getIp() >> 24) & 0xFF;
  }
  os << static_cast< int >(octets[0]) << "." << static_cast< int >(octets[1])
     << "." << static_cast< int >(octets[2]) << "."
     << static_cast< int >(octets[3]) << ":" << ntohs(addr.getPort());
  return os;
}

Ipv4Address::Ipv4Address(const std::string& address)
{
  u_int8_t ar[4];
  if (address.find_first_not_of("0123456789.:") != std::string::npos)
    throw std::runtime_error("Invalid Ip Address format");
  std::string::size_type pos = address.find(':');
  if (pos == std::string::npos)
    throw std::runtime_error("Invalid Ip Address format");
  std::string ip = address.substr(0, pos);
  std::stringstream ss(ip);
  std::string token;
  size_t i = 0;
  while (std::getline(ss, token, '.'))
  {
    if (i > 3)
      throw std::runtime_error("Invalid Ip Address format");
    ar[i++] = strtouint8(token);
  }
  if (i != 4)
    throw std::runtime_error("Invalid Ip Address format");
  std::string port = address.substr(pos + 1);
  port_ = strtouint16(port);
  port_ = htons(port_);
  ip_ = Utils::ipv4ToBigEndian(ar);
  type_ = IPv4;
  IpAddress::address_ = address;
  std::cout << *this << std::endl;
}

Ipv4Address::~Ipv4Address() {}

int Ipv4Address::createSocket() const
{
  struct sockaddr_in addr;

  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd == -1)
  {
    throw std::runtime_error("Unable to create socket");
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip_;
  addr.sin_port = port_;

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
  {
    close(fd);
    throw std::runtime_error("Unable to set socket options");
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    close(fd);
    throw std::runtime_error("Unable to bind socket to address");
  }

  return fd;
}

bool Ipv4Address::operator<(const IpAddress& other) const
{
  if (other.getType() == IPv6)
  {
    return true;
  }

  const Ipv4Address& converted = static_cast< const Ipv4Address& >(other);
  if (ip_ < converted.getIp() ||
      (ip_ == converted.getIp() && port_ < converted.getPort()))
  {
    return true;
  }

  return false;
}

bool Ipv4Address::operator==(const IpAddress& other) const
{
  if (other.getType() != IPv4)
  {
    return false;
  }
  const Ipv4Address& converted = static_cast< const Ipv4Address& >(other);
  if (converted.getIp() == ip_ && converted.getPort() == port_)
  {
    return true;
  }
  return false;
}

u_int32_t Ipv4Address::getIp() const
{
  return ip_;
}

u_int16_t Ipv4Address::getPort() const
{
  return port_;
}

#include <iostream>
int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " <ip_address>" << std::endl;
    return 1;
  }
  try
  {
    Ipv4Address addr(argv[1]);
  } catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
