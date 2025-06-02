#pragma once
#include <sys/types.h>
#include <string>
#include "IpAddress.hpp"

struct Ipv6
{
  u_int16_t ip[8];
};

bool operator<(const Ipv6& lhs, const Ipv6& rhs);

class Ipv6Address : public IpAddress
{
 public:
  // ── ◼︎ constructors and destructors ─────────────
  Ipv6Address(Ipv6 ip, u_int16_t port);
  Ipv6Address(u_int16_t port);
  Ipv6Address(const std::string& address);
  ~Ipv6Address();

  // ── ◼︎ operators ────────────────────────────────
  bool operator<(const IpAddress& other) const;
  bool operator==(const IpAddress& other) const;

  // ── ◼︎ member functions ─────────────────────────
  int createSocket() const;

  // ── ◼︎ getters ──────────────────────────────────
  const u_int16_t* getIp() const;
  u_int16_t getPort() const;

 private:
  u_int16_t ip_[8];
  u_int16_t port_;
};

std::ostream& operator<<(std::ostream& os, const Ipv6Address& addr);
