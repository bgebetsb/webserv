#pragma once

#include <string>

enum IpTypes
{
  IPv4,
  IPv6
};

class IpAddress
{
 public:
  IpAddress(const std::string& address, IpTypes type)
      : address_(address), type_(type)
  {}
  virtual ~IpAddress();
  virtual int createSocket() const = 0;

  virtual bool operator<(const IpAddress& other) const = 0;
  virtual bool operator==(const IpAddress& other) const = 0;
  IpTypes getType() const;
  const std::string& getOriginalAddress() const;

 protected:
  std::string address_;
  IpTypes type_;
};

std::ostream& operator<<(std::ostream& stream, const IpAddress& address);
