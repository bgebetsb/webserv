#include "ip/IpComparison.hpp"
#include "ip/IpAddress.hpp"

bool IpComparison::operator()(const IpAddress* lhs,
                              const IpAddress* rhs) const {
  return *lhs < *rhs;
}
