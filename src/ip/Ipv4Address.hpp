#include <sys/types.h>
#include "IpAddress.hpp"

#include <ostream>

class Ipv4Address : public IpAddress
{
 public:
  Ipv4Address(u_int32_t ip, u_int16_t port);
  ~Ipv4Address();

  int createSocket() const;
  bool operator<(const IpAddress& other) const;
  bool operator==(const IpAddress& other) const;
  u_int32_t getIp() const;
  u_int16_t getPort() const;

 private:
  u_int32_t ip_;
  u_int16_t port_;
};

std::ostream& operator<<(std::ostream& os, const Ipv4Address& addr);
