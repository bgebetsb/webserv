#include "Ipv4Connection.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include "exceptions/ConError.hpp"
#include "exceptions/FdLimitReached.hpp"
#include "utils/Utils.hpp"

Ipv4Connection::Ipv4Connection(int socket_fd,
                               const std::vector< Server >& servers)
    : Connection(servers)
{
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  fd_ = accept(socket_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
  if (fd_ == -1)
  {
    if (errno == EMFILE || errno == ENFILE)
      throw FdLimitReached("Unable to accept client connection");
    else
      throw ConErr("Unable to accept client connection");
  }

  if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1)
  {
    throw ConErr("Unable to set fd to non-blocking");
  }

  client_ip_ = Utils::ipv4ToString(peer_addr.sin_addr.s_addr);

  ep_event_->events = EPOLLIN | EPOLLRDHUP;

  ep_event_->data.ptr = this;
  request_ = Request(fd_, servers);
}

Ipv4Connection::~Ipv4Connection() {}
