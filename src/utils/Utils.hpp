#pragma once

#include <sys/types.h>
#include <string>

namespace Utils
{
  // ── ◼︎ Endian ─────────────────────────────────────
  std::string ipv4ToString(u_int32_t addr);
  std::string ipv6ToString(struct in6_addr& address);

  // ── ◼︎ String to int ──────────────────────────────
  u_int8_t ipStrToUint8(const std::string& str);
  u_int16_t ipStrToUint16(const std::string& str);
  u_int64_t strToMaxBodySize(const std::string& str);
  u_int16_t errStrToUint16(const std::string& str);

  // ── ◼︎ String utils ───────────────────────────────
  std::string trimString(const std::string& input);
  size_t countSubstr(const std::string& str, const std::string& substr);
  void toLower(char& c);
  void toUpperWithUnderscores(char& c);
  std::string replaceString(const std::string& input,
                            const std::string& search,
                            const std::string& replace);

  // Time utils
  u_int64_t getCurrentTime();

  // Fd Utils
  int addCloExecFlag(int fd);
  void ft_close(int& fd);
}  // namespace Utils
