#pragma once

#include "IpAddress.hpp"

struct IpComparison {
  bool operator()(const IpAddress* lhs, const IpAddress* rhs) const;
};
