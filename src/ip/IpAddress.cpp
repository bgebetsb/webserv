#include "IpAddress.hpp"

IpAddress::~IpAddress() {}

IpTypes IpAddress::getType() const
{
  return type_;
}

const std::string& IpAddress::getOriginalAddress() const
{
  return address_;
}

std::ostream& operator<<(std::ostream& stream, const IpAddress& address)
{
  stream << address.getOriginalAddress();
  return stream;
}
