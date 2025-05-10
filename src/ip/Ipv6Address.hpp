#pragma once
#include <sys/types.h>
#include "IpAddress.hpp"

#include <string>

struct PosDoubleColon;

class Ipv6Address : public IpAddress
{
 public:
  // ── ◼︎ constructors and destructors ─────────────
  Ipv6Address(u_int16_t port);
  PosDoubleColon checkDoubleColonPosition(const std::string& ip);
  void readBigEndianIpv6(const std::string& ip);
  Ipv6Address(const std::string& address);
  ~Ipv6Address();

  // ── ◼︎ operators ────────────────────────────────
  bool operator<(const IpAddress& other) const;
  bool operator==(const IpAddress& other) const;

  // ── ◼︎ member functions ───────────────────────
  int createSocket() const;

  // ── ◼︎ getters ────────────────────────────────
  const u_int16_t* getIp() const;
  u_int16_t getPort() const;

 private:
  u_int16_t ip_[8];
  u_int16_t port_;
};
