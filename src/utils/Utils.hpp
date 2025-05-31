#pragma once

#include <sys/types.h>
#include <string>

namespace Utils
{
  // ── ◼︎ Endian ─────────────────────────────────────
  bool isBigEndian();
  u_int16_t u16ToBigEndian(u_int16_t value);
  u_int32_t ipv4ToBigEndian(u_int8_t octets[4]);
  std::string ipv4ToString(u_int32_t addr);

  // ── ◼︎ String to int ──────────────────────────────
  u_int8_t ipStrToUint8(const std::string& str);
  u_int16_t ipStrToUint16(const std::string& str);
  u_int64_t strToMaxBodySize(const std::string& str);
  u_int16_t errStrToUint16(const std::string& str);
  int strToInt(const std::string& str);

  // ── ◼︎ String utils ───────────────────────────────
  std::string trimString(const std::string& input);
  size_t countSubstr(const std::string& str, const std::string& substr);
  void toLower(char& c);
  std::string replaceString(const std::string& input,
                            const std::string& search,
                            const std::string& replace);

  // Time utils
  size_t getCurrentTime();
}  // namespace Utils
