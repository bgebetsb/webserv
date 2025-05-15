#pragma once

#include <sys/types.h>
#include <string>

namespace Utils
{
  // ── ◼︎ validate ───────────────────────
  bool isValidUri(const std::string& uri);

  // ── ◼︎ Conversion ────────────────────────
  u_int32_t ipStrToUint32Max(const std::string& str, u_int32_t max);

}  // namespace Utils
