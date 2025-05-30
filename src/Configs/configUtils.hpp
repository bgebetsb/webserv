#pragma once

#include <sys/types.h>
#include <string>
#include <vector>

namespace Utils
{
  // ── ◼︎ validate ───────────────────────
  bool isValidUri(const std::string& uri);
  bool isValidHostname(const std::string& hostname);
  bool duplicateEntries(const std::vector< std::string >& entries);
  // ── ◼︎ Conversion ────────────────────────
  u_int32_t ipStrToUint32Max(const std::string& str, u_int32_t max);

}  // namespace Utils
