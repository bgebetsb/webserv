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

Ipv6Address::Ipv6Address(u_int16_t port)
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
      pos.blocks_before = Utils::countSubstr(before, ":") + 1;
      pos.blocks_after = Utils::countSubstr(after, ":") + 1;
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

void Ipv6Address::readBigEndianIpv6(const std::string& ip)
{
  std::cout << "readBigEndianIpv6: " << ip << std::endl;
  std::stringstream ss(ip);
  std::string item;
  PosDoubleColon DCPos = checkDoubleColonPosition(ip);
  int i = 0;
  while (std::getline(ss, item, ':'))
  {
    std::cout << "item: " << item << std::endl;
    if (i > 7 || item.length() > 4)
      throw std::runtime_error("Invalid Ipv6 Address format");
    std::stringstream hex(item);
    if (item.length() == 0)
    {
      ++i;
      continue;
    }
    hex >> std::hex >> ip_[i];
    if (hex.fail())
      throw std::runtime_error("Invalid Ipv6 Address format");
    ip_[i] = htons(ip_[i]);
    ++i;
  }
  int missing = 8 - (i + 1);
  if (missing && DCPos.type != POS_NONE)
  {
    if (DCPos.type == POS_BEFORE)
    {
      std::memmove(ip_ + missing, ip_, (8 - missing) * sizeof(u_int16_t));
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
  } else if (missing)
  {
    throw std::runtime_error("Invalid Ipv6 Address format");
  }
}

Ipv6Address::Ipv6Address(const std::string& address)
{
  if (address.find_first_not_of("0123456789abcdef:[]") != std::string::npos ||
      address[0] != '[')
    throw std::runtime_error("Invalid Ipv6 Address format");
  std::string::size_type pos = address.find(']');
  if (pos == std::string::npos || pos <= 1)
    throw std::runtime_error("Invalid Ipv6 Address format");
  std::string ip = address.substr(1, pos - 1);
  std::string port = address.substr(pos + 1);
  if (port[0] != ':')
    throw std::runtime_error("Invalid Ipv6 Address format");
  port_ = Utils::strtouint16(port.substr(1));
  port_ = Utils::u16ToBigEndian(port_);
  readBigEndianIpv6(ip);
  std::cout << *this << std::endl;
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

#include <iostream>
#include <stdexcept>
#include <vector>
#include "Ipv6Address.hpp"

int main()
{
  try
  {
    std::cout << "=== Gültige Adressen ===" << std::endl;

    Ipv6Address addr1("[2001:0db8::1]:8080");
    Ipv6Address addr2("[2001:0db8::1]:8080");
    Ipv6Address addr3("[2001:0db8:0000:0000:0000:0000:0000:0002]:8080");
    Ipv6Address addr4("[::1]:80");
    Ipv6Address addr5("[::]:443");
    Ipv6Address addr6(12345);  // default-IP (::) mit nur Port

    std::cout << "addr1: " << addr1 << std::endl;
    std::cout << "addr2: " << addr2 << std::endl;
    std::cout << "addr3: " << addr3 << std::endl;
    std::cout << "addr4: " << addr4 << std::endl;
    std::cout << "addr5: " << addr5 << std::endl;
    std::cout << "addr6: " << addr6 << std::endl;

    std::cout << "\n=== Vergleiche ===" << std::endl;

    std::cout << "addr1 == addr2: " << (addr1 == addr2 ? "true" : "false")
              << std::endl;
    std::cout << "addr1 == addr3: " << (addr1 == addr3 ? "true" : "false")
              << std::endl;
    std::cout << "addr1 < addr3 : " << (addr1 < addr3 ? "true" : "false")
              << std::endl;
    std::cout << "addr3 < addr1 : " << (addr3 < addr1 ? "true" : "false")
              << std::endl;

    std::cout << "\n=== Ungültige Adressen ===" << std::endl;

    std::vector< std::string > invalid_inputs;
    invalid_inputs.push_back("2001::1:8080");        // fehlt [ ]
    invalid_inputs.push_back("[2001::1]8080");       // fehlt :
    invalid_inputs.push_back("[2001::1]:");          // kein Port
    invalid_inputs.push_back("[2001::1]:0");         // Port = 0
    invalid_inputs.push_back("[2001::1::abcd]:80");  // doppeltes ::
    invalid_inputs.push_back("[2001:zzz::1]:80");    // ungültiges Hex

    for (size_t i = 0; i < invalid_inputs.size(); ++i)
    {
      try
      {
        Ipv6Address failtest(invalid_inputs[i]);
        std::cerr
            << "❌ Fehler: Eingabe akzeptiert, sollte aber ungültig sein: "
            << invalid_inputs[i] << std::endl;
      } catch (const std::exception& e)
      {
        std::cout << "✅ Ausnahme korrekt geworfen für '" << invalid_inputs[i]
                  << "': " << e.what() << std::endl;
      }
    }

  } catch (const std::exception& ex)
  {
    std::cerr << "❌ Unerwarteter Fehler: " << ex.what() << std::endl;
    return 1;
  }

  std::cout << "\n✅ Alle Tests abgeschlossen.\n";
  return 0;
}
