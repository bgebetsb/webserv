#pragma once

#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <vector>
#include "Configs/Configs.hpp"
#include "epoll/EpollData.hpp"
#include "epoll/Listener.hpp"
#include "ip/IpAddress.hpp"
#include "ip/IpComparison.hpp"

typedef int filedescriptor;
typedef std::set< IpAddress*, IpComparison > IpSet;
typedef std::map< const IpAddress*, filedescriptor, IpComparison > ListenerMap;
typedef std::vector< Server > VServers;
typedef std::multimap< u_int64_t, int, std::greater< u_int64_t > > MMKeepAlive;

class Webserv
{
 public:
  Webserv(std::string config_file = "webserv.conf",
          Configuration& config = Configuration::getInstance());
  ~Webserv();

  void initialize_servas();
  void addServer(const IpSet& listeners, const Server& server);
  void mainLoop();

 private:
  ListenerMap listeners_;
  EpollData& ed_;
  struct epoll_event* events_;
  VServers servers_;
  Configuration& config_;

  // Copy constructor and copy assignment are unused anyway, thus private
  Webserv(const Server& other);
  Webserv& operator=(const Webserv& other);

  Listener& getListener(IpAddress* addr);
  void addServers();
  void addFdsToEpoll() const;
  void addFd(int fd, struct epoll_event* event);
  void modifyFd(int fd, struct epoll_event* event) const;
  void deleteFd(int fd);
  void pingAllClients(size_t needed_fds);
  void closeClientConnections(const MMKeepAlive& fds, size_t needed_fds);
};
