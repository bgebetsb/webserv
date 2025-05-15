#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <csignal>
#include <cstddef>
#include <exception>
#include <iostream>
#include "Webserv.hpp"
#include "exceptions/Fatal.hpp"
#include "ip/IpAddress.hpp"
#include "ip/Ipv4Address.hpp"

// struct sockaddr_in {
//     sa_family_t     sin_family;     /* AF_INET */
//     in_port_t       sin_port;       /* Port number */
//     struct in_addr  sin_addr;       /* IPv4 address */
// };
//
// struct sockaddr_in6 {
//     sa_family_t     sin6_family;    /* AF_INET6 */
//     in_port_t       sin6_port;      /* Port number */
//     uint32_t        sin6_flowinfo;  /* IPv6 flow info */
//     struct in6_addr sin6_addr;      /* IPv6 address */
//     uint32_t        sin6_scope_id;  /* Set of interfaces for a scope */
// };
//
// struct in_addr {
//     in_addr_t s_addr;
// };
//
// struct in6_addr {
//     uint8_t   s6_addr[16];
// };

#include <vector>

using std::pair;
using std::vector;

volatile sig_atomic_t g_signal = 0;

void handle_signal(int signum)
{
  g_signal = signum;
}

void setup_signals(void)
{
  int catch_signals[] = {SIGINT, SIGQUIT, SIGHUP, SIGTERM, 0};
  int ignore_signals[] = {SIGUSR1, SIGUSR2, 0};

  for (size_t i = 0; catch_signals[i]; ++i)
  {
    signal(catch_signals[i], handle_signal);
  }

  for (size_t i = 0; ignore_signals[i]; ++i)
  {
    signal(ignore_signals[i], SIG_IGN);
  }
}

vector< pair< vector< IpAddress* >, Server > > createTestServers()
{
  // u_int16_t port = htons(8080);
  // u_int8_t ar[4] = {127, 0, 0, 1};
  // u_int32_t ip = Utils::ipv4ToBigEndian(ar);

  IpAddress* ip = new Ipv4Address("127.0.0.1:8080");
  Server server;

  vector< IpAddress* > listeners;
  listeners.push_back(ip);

  pair< vector< IpAddress* >, Server > pair(listeners, server);
  vector< ::pair< vector< IpAddress* >, Server > > ret;
  ret.push_back(pair);

  return ret;
}

int main(int argc, char* argv[])
try
{
  setup_signals();
  // TODO: Read config file
  try
  {
    if (argc > 2)
    {
      std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
      return 1;
    } else if (argc == 2)
    {
      Webserv w(argv[1]);
      w.mainLoop();
    } else
    {
      Webserv w;
      w.mainLoop();
    }
  } catch (std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
} catch (...)
{}
