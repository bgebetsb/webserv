#pragma once

#include <sys/types.h>
#include <string>

namespace Utils
{
  // ── ◼︎ Endian ─────────────────────────────────────
  bool isBigEndian();
  u_int16_t u16ToBigEndian(u_int16_t value);
  u_int32_t ipv4ToBigEndian(u_int8_t octets[4]);

  // ── ◼︎ String to int ──────────────────────────────
  u_int8_t strtouint8(const std::string& str);
  u_int16_t strtouint16(const std::string& str);

  // ── ◼︎ String utils ───────────────────────────────
  std::string trimString(const std::string& input);
  size_t countSubstr(const std::string& str, const std::string& substr);

  // Time utils
  size_t getCurrentTime();
}  // namespace Utils
