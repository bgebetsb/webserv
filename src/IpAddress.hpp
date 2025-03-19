#pragma once

enum IpTypes {
  IPv4,
  IPv6
};

class IpAddress {
 public:
  virtual ~IpAddress() = 0;
  IpTypes getIpType() const;

 private:
  IpTypes type_;
};
