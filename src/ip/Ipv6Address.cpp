#include "Ipv6Address.hpp"
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../exceptions/Fatal.hpp"
#include "IpAddress.hpp"

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Operators              ║
// ╚══════════════════════════════════════════════╝
bool operator<(const Ipv6& lhs, const Ipv6& rhs)
{
  for (unsigned int i = 0; i < 8; ++i)
  {
    if (lhs.ip[i] < rhs.ip[i])
      return true;
  }
  return false;
}

bool Ipv6Address::operator<(const IpAddress& other) const
{
  if (other.getType() != IPv6)
    return false;

  const Ipv6Address& converted = static_cast< const Ipv6Address& >(other);
  const u_int16_t* ip = converted.getIp();

  for (size_t i = 0; i < 8; ++i)
  {
    if (ip_[i] < ip[i])
      return true;
    else if (ip_[i] > ip[i])
      return false;
  }

  return port_ < converted.getPort();
}

bool Ipv6Address::operator==(const IpAddress& other) const
{
  if (other.getType() != IPv6)
  {
    return false;
  }
  const Ipv6Address& converted = static_cast< const Ipv6Address& >(other);
  const u_int16_t* ip = converted.getIp();
  if (converted.getPort() == port_)
  {
    for (size_t i = 0; i < 8; ++i)
    {
      if (ip[i] != ip_[i])
        return false;
    }
    return true;
  }
  return false;
}

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Member functions       ║
// ╚══════════════════════════════════════════════╝

Ipv6Address::Ipv6Address(Ipv6 ip, u_int16_t port, const std::string& original)
    : IpAddress(original, IPv6)
{
  if (port == 0)
    throw Fatal("Invalid Ipv6 Address format");
  std::memcpy(ip_, ip.ip, 16);
  port_ = htons(port);
}

Ipv6Address::~Ipv6Address() {}

int Ipv6Address::createSocket() const
{
  struct sockaddr_in6 addr;

  int fd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd == -1)
  {
    throw Fatal("Unable to create socket");
  }
  std::memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  std::memcpy(&addr.sin6_addr, ip_, 16);
  addr.sin6_port = port_;

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
  {
    close(fd);
    throw Fatal("Unable to set socket options");
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    close(fd);
    throw Fatal("Unable to bind socket to address");
  }

  return fd;
}

// ╔══════════════════════════════════════════════╗
// ║              SECTION: getters                ║
// ╚══════════════════════════════════════════════╝

const u_int16_t* Ipv6Address::getIp() const
{
  return ip_;
}

u_int16_t Ipv6Address::getPort() const
{
  return port_;
}
