#include "Webserv.hpp"
#include <algorithm>
#include "Listener.hpp"
#include "Server.hpp"

Webserv::Webserv() {}

void Webserv::addServer(const std::vector< Listener >& listeners,
                        const Server& server) {
  typedef std::vector< Listener >::const_iterator iter_type;

  for (iter_type it = listeners.begin(); it < listeners.end(); it++) {
    std::vector< Listener >::iterator listener =
        std::find(listeners_.begin(), listeners_.end(), *it);
    if (listener == listeners_.end()) {
      listeners_.push_back(*it);
      listener = listeners_.end() - 1;
    }
    listener->addServer(server);
  }
}
