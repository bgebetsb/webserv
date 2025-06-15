#include "Ipv4Address.hpp"
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include "../exceptions/Fatal.hpp"
#include "IpAddress.hpp"

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Operators              ║
// ╚══════════════════════════════════════════════╝

bool Ipv4Address::operator<(const IpAddress& other) const
{
  if (other.getType() == IPv6)
    return true;

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

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Member functions       ║
// ╚══════════════════════════════════════════════╝

Ipv4Address::Ipv4Address(u_int32_t ip,
                         u_int16_t port,
                         const std::string& original)
    : IpAddress(original, IPv4)
{
  if (port == 0)
    throw Fatal("Invalid Ipv4 Address format");
  ip_ = ip;
  port_ = htons(port);
}

Ipv4Address::~Ipv4Address() {}

int Ipv4Address::createSocket() const
{
  struct sockaddr_in addr;

  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd == -1)
  {
    throw Fatal("Unable to create socket");
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip_;
  addr.sin_port = port_;

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

u_int32_t Ipv4Address::getIp() const
{
  return ip_;
}

u_int16_t Ipv4Address::getPort() const
{
  return port_;
}
