#include "IpAddress.hpp"

IpAddress::~IpAddress() {}

IpTypes detectType(const std::string& input)
{
  // TODO: Actually check and return the correct type here
  (void)input;
  return IPv4;
}

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
