#include "ip/Ipv4Address.hpp"
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include "ip/IpAddress.hpp"
#include "utils/Utils.hpp"

Ipv4Address::Ipv4Address(const std::string& address) {
  // TODO: Actually parse the address
  (void)address;
  u_int16_t port = htons(8080);
  u_int8_t ar[4] = {127, 0, 0, 1};
  u_int32_t ip = Utils::ipv4ToBigEndian(ar);

  ip_ = ip;
  port_ = port;
  IpAddress::address_ = address;
}

Ipv4Address::~Ipv4Address() {}

int Ipv4Address::createSocket() const {
  struct sockaddr_in addr;

  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (fd == -1) {
    throw std::runtime_error("Unable to create socket");
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ip_;
  addr.sin_port = port_;

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    close(fd);
    throw std::runtime_error("Unable to set socket options");
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    close(fd);
    throw std::runtime_error("Unable to bind socket to address");
  }

  return fd;
}
