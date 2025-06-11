#pragma once

#include <sys/types.h>
#include <set>
#include <string>
#include <vector>
#include "ip/Ipv6Address.hpp"

namespace Utils
{
  // ── ◼︎ validate ───────────────────────
  bool isValidUri(const std::string& uri);
  bool duplicateEntries(const std::vector< std::string >& entries);
  // ── ◼︎ Conversion ────────────────────────
  u_int32_t ipStrToUint32Max(const std::string& str, u_int32_t max);

  std::set< u_int32_t > getIpv4Addresses(const std::string& ip_string);
  std::set< Ipv6 > getIpv6Addresses(const std::string& ip_string);

}  // namespace Utils
