#include "PidTracker.hpp"
#include "epoll/EpollData.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include "Configs/Configs.hpp"
#include "Logger/Logger.hpp"
#include "Webserv.hpp"
#include "epoll/Connection.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollFd.hpp"
#include "epoll/Listener.hpp"
#include "exceptions/ConError.hpp"
#include "exceptions/Fatal.hpp"
#include "exceptions/FdLimitReached.hpp"
#include "ip/IpAddress.hpp"

#define MAX_EVENTS 1024

#define KEEPALIVE_TIMEOUT_SECONDS 30

/*
 * The `size` argument in epoll_create is just for backwards compatibility.
 * Doesn't do anything since Linux kernel v2.6.8, only needs to be greater
 * than zero.
 */
Webserv::Webserv(std::string config_file, Configuration& config)
    : ed_(getEpollData()), events_(NULL), config_(config)
{
  try
  {
    events_ = new struct epoll_event[MAX_EVENTS];
    config_.parseConfigFile(config_file);
  }
  catch (const Fatal& e)
  {
    if (events_)
      delete[] events_;
    throw;
  }
  servers_ = config_.getServerConfigs();
}

Webserv::~Webserv()
{
  delete[] events_;
}

void Webserv::addServer(const IpSet& listeners, const Server& server)
{
  typedef std::set< IpAddress* >::const_iterator iter_type;

  for (iter_type it = listeners.begin(); it != listeners.end(); ++it)
  {
    Listener& listener = getListener(*it);
    listener.addServer(server);
  }
}

Listener& Webserv::getListener(IpAddress* addr)
{
  Listener* listener;

  std::map< const IpAddress*, int >::iterator fd_it = listeners_.find(addr);

  if (fd_it != listeners_.end())
  {
    listener = static_cast< Listener* >(ed_.fds[fd_it->second]);
  }
  else
  {
    listener = new Listener(addr);
    int fd = listener->getFd();
    listeners_[addr] = fd;
    ed_.fds[fd] = listener;
  }

  return *listener;
}

void Webserv::addFd(int fd, struct epoll_event* event)
{
  if (epoll_ctl(ed_.fd, EPOLL_CTL_ADD, fd, event) == -1)
  {
    throw std::runtime_error("Unable to add fd to epoll");
  }

  ed_.fds[fd] = static_cast< EpollFd* >(event->data.ptr);
}

void Webserv::modifyFd(int fd, struct epoll_event* event) const
{
  if (epoll_ctl(ed_.fd, EPOLL_CTL_MOD, fd, event) == -1)
  {
    throw std::runtime_error("Unable to modify epoll event");
  }
}

void Webserv::deleteFd(int fd)
{
  if (epoll_ctl(ed_.fd, EPOLL_CTL_DEL, fd, NULL) == -1)
  {
    throw std::runtime_error("Unable to remove fd from epoll");
  }

  delete ed_.fds[fd];
  ed_.fds.erase(fd);
}

void Webserv::addServers()
{
  for (size_t i = 0; i < servers_.size(); ++i)
  {
    std::cout << "Server " << i << ": " << servers_[i] << std::endl;
    const Server& server = servers_[i];
    addServer(server.ips, server);
  }
}

void Webserv::addFdsToEpoll() const
{
  typedef std::map< int, EpollFd* >::const_iterator iter_type;

  for (iter_type it = ed_.fds.begin(); it != ed_.fds.end(); ++it)
  {
    epoll_ctl(ed_.fd, EPOLL_CTL_ADD, it->first, it->second->getEvent());
  }
}

void Webserv::mainLoop()
{
  extern volatile sig_atomic_t g_signal;

  addServers();
  addFdsToEpoll();

  const LogSettings& logsettings = config_.getAccessLogsettings();
  if (logsettings.configured && logsettings.mode == LOGFILE)
    Logger::openFile(logsettings.logfile);
  else if (logsettings.configured)
    Logger::setLogMode(logsettings.mode);

  while (true)
  {
    int count = epoll_wait(ed_.fd, events_, MAX_EVENTS, 1000);

    if (g_signal || count == -1)
    {
      std::cerr << "\nSignal received, shutdown server\n";
      break;
    }

    size_t needed_fds = 0;

    for (int j = 0; j < count; ++j)
    {
      EpollFd* fd = static_cast< EpollFd* >(events_[j].data.ptr);

      try
      {
        EpollAction action = fd->epollCallback(events_[j].events);

        switch (action.op)
        {
          case EPOLL_ACTION_ADD:
            addFd(action.fd, action.event);
            break;
          case EPOLL_ACTION_MOD:
            modifyFd(action.fd, action.event);
            break;
          case EPOLL_ACTION_DEL:
            deleteFd(action.fd);
            break;
          default:;  // Do nothing on EPOLL_ACTION_UNCHANGED
        }
      }
      catch (FdLimitReached& e)
      {
        std::cerr << e.what() << "\n";
        needed_fds++;
      }
      catch (ConErr& e)
      {
        deleteFd(fd->getFd());
      }
      catch (std::bad_alloc& e)
      {
        Connection* c = dynamic_cast< Connection* >(fd);
        if (c)
          deleteFd(c->getFd());
      }
    }

    PidTracker& pidtracker = getPidTracker();
    pidtracker.ping();
    pingAllClients(needed_fds);
  }
}

/*
 * Sends a ping to every connected client, sets responses to timeout messages if
 * needed (if it takes too long to send the headers). If a client is currently
 * in keep-alive state, it will be pushed onto a multimap, and it will be closed
 * if a) it is in keep-alive state for too long, or b) the server needs more fds
 * (it will close the fds who are in keepalive state the longest first).
 */
void Webserv::pingAllClients(size_t needed_fds)
{
  EpollMap::iterator it;
  MMKeepAlive keepalive_fds;

  for (it = ed_.fds.begin(); it != ed_.fds.end(); ++it)
  {
    Connection* c = dynamic_cast< Connection* >(it->second);
    if (c)
    {
      std::pair< EpollAction, u_int64_t > action_time = c->ping();
      if (action_time.first.op == EPOLL_ACTION_MOD)
      {
        modifyFd(action_time.first.fd, action_time.first.event);
        continue;
      }

      if (action_time.second > 0)
      {
        keepalive_fds.insert(
            MMKeepAlive::value_type(action_time.second, action_time.first.fd));
      }
    }
  }

  closeClientConnections(keepalive_fds, needed_fds);
}

void Webserv::closeClientConnections(const MMKeepAlive& keepalive_fds,
                                     size_t needed_fds)
{
  MMKeepAlive::const_iterator it = keepalive_fds.begin();
  size_t total_closed = 0;
  size_t time_limit = config_.getKeepAliveTimeout();

  while (it != keepalive_fds.end() &&
         (it->first > time_limit || total_closed < needed_fds))
  {
    std::cerr << "Closing fd " << it->second;
    if (it->first > time_limit)
      std::cerr << "(Keepalive timeout reached)\n";
    else
      std::cerr << "(Keepalive state and more fds needed)\n";
    deleteFd(it->second);
    total_closed++;
    ++it;
  }
}
