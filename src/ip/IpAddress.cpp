#include "IpAddress.hpp"

IpAddress::~IpAddress() {}

IpTypes detectType(const std::string& input) {
  // TODO: Actually check and return the correct type here
  (void)input;
  return IPv4;
}
