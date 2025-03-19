#include "IpAddress.hpp"

IpAddress::~IpAddress() {}

IpTypes IpAddress::getIpType() const {
  return type_;
}
