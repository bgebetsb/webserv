#include <sys/types.h>
#include "IpAddress.hpp"

#include <string>

class Ipv6Address : public IpAddress
{
 public:
  Ipv6Address(u_int16_t port);
  Ipv6Address(const std::string& address);
  ~Ipv6Address();

  int createSocket() const;
  bool operator<(const IpAddress& other) const;
  bool operator==(const IpAddress& other) const;
  u_int32_t getIp() const;
  u_int16_t getPort() const;

 private:
  u_int16_t ip_[8];
  u_int16_t port_;
};
