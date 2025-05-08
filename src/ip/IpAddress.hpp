#pragma once

#include <string>

enum IpTypes {
  IPv4,
  IPv6
};

class IpAddress {
 public:
  virtual ~IpAddress();
  virtual int createSocket() const = 0;

  static IpTypes detectType(const std::string& input);
  virtual bool operator<(const IpAddress& other) const = 0;
  IpTypes getType() const;

 protected:
  std::string address_;
  IpTypes type_;
};
