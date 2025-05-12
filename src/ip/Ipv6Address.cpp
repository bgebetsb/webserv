#include "Ipv6Address.hpp"
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include "../utils/Utils.hpp"
#include "IpAddress.hpp"

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Operators              ║
// ╚══════════════════════════════════════════════╝

std::ostream& operator<<(std::ostream& os, const Ipv6Address& addr)
{
  (void)addr;
  os << "[";
  const u_int16_t* ip = addr.getIp();
  for (size_t i = 0; i < 8; ++i)
  {
    os << std::hex << ntohs(ip[i]);
    if (i != 7)
      os << ":";
  }
  os << "]:";
  os << std::dec << ntohs(addr.getPort());
  return os;
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

Ipv6Address::Ipv6Address(u_int16_t port) : IpAddress("", IPv6)
{
  if (port == 0)
    throw std::runtime_error("Invalid Ipv6 Address format");
  std::memset(ip_, 0, sizeof(uint16_t) * 8);
  port_ = htons(port);
}

enum PosType
{
  POS_BEFORE,
  POS_MIDDLE,
  POS_AFTER,
  POS_NONE
};

struct PosDoubleColon
{
  PosType type;
  size_t blocks_before;
  size_t blocks_after;
  size_t blocks_to_fill;
};

PosDoubleColon Ipv6Address::checkDoubleColonPosition(const std::string& ip)
{
  PosDoubleColon pos;

  std::string::size_type posdoublecolon = ip.find("::");

  if (posdoublecolon != std::string::npos)
  {
    if (ip.find("::", posdoublecolon + 2) != std::string::npos)
      throw std::runtime_error("Invalid Ipv6 Address format");
    else
    {
      std::string before = ip.substr(0, posdoublecolon);
      std::string after = ip.substr(posdoublecolon + 2);
      pos.blocks_before =
          before.empty() ? 0 : Utils::countSubstr(before, ":") + 1;
      pos.blocks_after = after.empty() ? 0 : Utils::countSubstr(after, ":") + 1;
      if (pos.blocks_before == 0)
        pos.type = POS_BEFORE;
      else if (pos.blocks_after == 0)
        pos.type = POS_AFTER;
      else
        pos.type = POS_MIDDLE;
      if (pos.blocks_before + pos.blocks_after > 7)
        throw std::runtime_error("Invalid Ipv6 Address format");
      pos.blocks_to_fill = 8 - (pos.blocks_before + pos.blocks_after);
      return pos;
    }
  }
  pos.type = POS_NONE;
  return pos;
}

void Ipv6Address::readBigEndianIpv6(const std::string& ip_string)
{
  // TODO: std::cout << "readBigEndianIpv6: " << ip_string << std::endl;
  std::stringstream ss(ip_string);
  std::string item;
  PosDoubleColon DCPos = checkDoubleColonPosition(ip_string);
  int i = 0;
  while (std::getline(ss, item, ':'))
  {
    // TODO: std::cout << "item: " << item << std::endl;
    if (i > 7 || item.length() > 4)
      throw std::runtime_error("Invalid Ipv6 Address format");
    std::stringstream hex(item);
    if (item.length() == 0)
      continue;
    hex >> std::hex >> ip_[i];
    if (hex.fail())
      throw std::runtime_error("Invalid Ipv6 Address format");
    ip_[i] = htons(ip_[i]);
    ++i;
  }
  if (DCPos.type != POS_NONE)
  {
    if (DCPos.type == POS_BEFORE)
    {
      std::memmove(ip_ + DCPos.blocks_before, ip_,
                   DCPos.blocks_to_fill * sizeof(u_int16_t));
      std::memset(ip_, 0, DCPos.blocks_to_fill * sizeof(u_int16_t));
    } else if (DCPos.type == POS_AFTER)
      std::memset(ip_ + DCPos.blocks_before, 0,
                  DCPos.blocks_to_fill * sizeof(u_int16_t));
    else if (DCPos.type == POS_MIDDLE)
    {
      std::memmove(
          ip_ + DCPos.blocks_before + DCPos.blocks_to_fill,
          ip_ + DCPos.blocks_before,
          (8 - DCPos.blocks_before - DCPos.blocks_to_fill) * sizeof(u_int16_t));
      std::memset(ip_ + DCPos.blocks_before, 0,
                  DCPos.blocks_to_fill * sizeof(u_int16_t));
    }
  } else if (i != 8)
  {
    throw std::runtime_error("Invalid Ipv6 Address format");
  }
}

Ipv6Address::Ipv6Address(const std::string& address) : IpAddress(address, IPv6)
{
  std::memset(ip_, 0, sizeof(uint16_t) * 8);
  if (address.empty() ||
      address.find_first_not_of("0123456789abcdef:[]") != std::string::npos ||
      address[0] != '[')
    throw std::runtime_error("Invalid Ipv6 Address format");
  std::string::size_type pos = address.find(']');
  if (pos == std::string::npos || pos <= 1)
    throw std::runtime_error("Invalid Ipv6 Address format");
  std::string ip = address.substr(1, pos - 1);
  std::string port = address.substr(pos + 1);
  if (port[0] != ':' || ip.empty())
    throw std::runtime_error("Invalid Ipv6 Address format");
  port_ = Utils::strtouint16(port.substr(1));
  port_ = Utils::u16ToBigEndian(port_);
  if (port_ == 0)
    throw std::runtime_error("Invalid Ipv6 Address format");
  readBigEndianIpv6(ip);
}

Ipv6Address::~Ipv6Address() {}

// TODO: implement createSocket

int Ipv6Address::createSocket() const
{
  // struct sockaddr_in addr;

  // int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  // if (fd == -1)
  // {
  //   throw std::runtime_error("Unable to create socket");
  // }
  // addr.sin_family = AF_INET;
  // addr.sin_addr.s_addr = ip_;
  // addr.sin_port = port_;

  // int reuse = 1;
  // if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
  // {
  //   close(fd);
  //   throw std::runtime_error("Unable to set socket options");
  // }

  // if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  // {
  //   close(fd);
  //   throw std::runtime_error("Unable to bind socket to address");
  // }

  // return fd;
  return 0;  // Placeholder for the actual socket creation
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
