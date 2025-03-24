#include <sys/types.h>
#include "IpAddress.hpp"

#include <string>

class Ipv4Address : public IpAddress {
 public:
  Ipv4Address(const std::string& address);
  ~Ipv4Address();

  int createSocket() const;

 private:
  u_int32_t ip_;
  u_int16_t port_;
};
