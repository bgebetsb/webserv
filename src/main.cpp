#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <csignal>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include "Listener.hpp"
#include "Webserv.hpp"
#include "utils/Utils.hpp"

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

#define MAX_EVENTS 1024

using std::pair;
using std::vector;

volatile sig_atomic_t g_signal = 0;

void handle_signal(int signum) {
  g_signal = signum;
}

void setup_signals(void) {
  int catch_signals[] = {SIGINT, SIGQUIT, SIGHUP, SIGTERM, 0};
  int ignore_signals[] = {SIGUSR1, SIGUSR2, 0};

  for (size_t i = 0; catch_signals[i]; i++) {
    signal(catch_signals[i], handle_signal);
  }

  for (size_t i = 0; ignore_signals[i]; i++) {
    signal(ignore_signals[i], SIG_IGN);
  }
}

vector< pair< vector< Listener >, Server > > createTestServers() {
  u_int16_t port = htons(8080);
  u_int8_t ar[4] = {127, 0, 0, 1};
  u_int32_t ip = Utils::ipv4ToBigEndian(ar);

  Listener listener(ip, port);
  Server server;

  vector< Listener > listeners;
  listeners.push_back(listener);

  pair< vector< Listener >, Server > pair(listeners, server);
  vector< ::pair< vector< Listener >, Server > > ret;
  ret.push_back(pair);

  return ret;
}

int main(int argc, char* argv[]) {
  // TODO: Read config file
  (void)argc;
  (void)argv;

  u_int16_t port = htons(8080);
  u_int8_t ar[4] = {127, 0, 0, 1};
  u_int32_t ip = Utils::ipv4ToBigEndian(ar);

  struct epoll_event ep_event2;
  ep_event2.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
  struct epoll_event* events = new struct epoll_event[MAX_EVENTS];

  setup_signals();

  vector< pair< vector< Listener >, Server > > testData = createTestServers();

  typedef vector< pair< vector< Listener >, Server > >::iterator it_type;
  Webserv w;
  try {
    for (it_type it = testData.begin(); it < testData.end(); it++) {
      w.addServer(it->first, it->second);
    }
    Listener listener(ip, port);

    int fd = listener.listen();
    /*
     * The `size` argument in epoll_create is just for backwards compatibility.
     * Doesn't do anything since Linux kernel v2.6.8, only needs to be greater
     * than zero.
     */
    int epoll_fd = epoll_create(1024);
    if (epoll_fd == -1) {
      throw std::runtime_error("Unable to create epoll fd");
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, listener.getEpollEvent()) < 0) {
      close(epoll_fd);
      throw std::runtime_error("Unable to add fd to epoll fd");
    }
    while (1) {
      int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

      if (g_signal || count == -1) {
        std::cerr << "Signal received, shutdown server\n";
        break;
      }

      // std::cout << count << " fds are ready\n";

      for (int j = 0; j < count; j++) {
        if (events[j].data.fd == fd) {
          std::cout << "Trying to accept new fd\n";
          struct sockaddr_in peer_addr;
          socklen_t peer_addr_size = sizeof(peer_addr);
          int cfd = accept(fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
          if (cfd == -1) {
            std::cerr << "Accept failure\n";
            return 1;
          }
          std::cout << "Success\n";
          ep_event2.data.fd = cfd;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ep_event2);
          continue;
        }
        // std::cout << "FD: " << events[j].data.fd << "\n";
        // std::cout << "POLLIN: " << (events[j].events & EPOLLIN) << "\n";
        // std::cout << "POLLOUT: " << (events[j].events & EPOLLOUT) << "\n";
        if (events[j].events & EPOLLRDHUP) {
          std::cout << "FD disconnected: " << events[j].data.fd << "\n";
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[j].data.fd, NULL);
        }
        // std::cout << "EPOLLRDHUP: " << (events[j].events & EPOLLRDHUP) <<
        // "\n";
      }
    }

    close(epoll_fd);

  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
  }
}
