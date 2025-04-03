#include "Connection.hpp"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <set>
#include <stdexcept>
#include <string>
#include "epoll/EpollAction.hpp"
#include "requests/RequestStatus.hpp"

Connection::Connection(int socket_fd, const std::set< Server >& servers)
    : servers_(servers), polling_write_(false) {
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  readbuf_ = new char[CHUNK_SIZE];
  fd_ = accept(socket_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
  if (fd_ == -1) {
    throw std::runtime_error("Unable to accept client connection");
  }

  ep_event_->events = EPOLLIN | EPOLLRDHUP;

  ep_event_->data.ptr = this;
  requests_.push_back(Request(fd_));
}

Connection::~Connection() {
  delete[] readbuf_;
}

EpollAction Connection::epollCallback(int event) {
  if (event & EPOLLIN) {
    return handleRead();
  } else if (event & EPOLLOUT) {
    return handleWrite();
  }

  EpollAction action = {fd_, EPOLL_ACTION_DEL, NULL};
  return action;
}

EpollAction Connection::handleRead() {
  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, NULL};

  ssize_t ret = recv(fd_, readbuf_, 1024, 0);
  if (ret == -1) {
    throw std::runtime_error("Recv failed");
  }
  buffer_.append(readbuf_, ret);
  size_t pos = buffer_.find('\n');
  while (pos != std::string::npos) {
    size_t realpos = pos;
    if (realpos > 0 && buffer_[realpos - 1] == '\r') {
      --realpos;
    }
    std::string line(buffer_, 0, realpos);
    if (buffer_.size() > pos + 1) {
      buffer_ = std::string(buffer_, pos + 1);
    } else {
      buffer_.clear();
    }
    requests_.back().addHeaderLine(line);

    if (requests_.back().getStatus() == SENDING_RESPONSE ||
        requests_.back().getStatus() == COMPLETED) {
      requests_.push_back(Request(fd_));
    }

    pos = buffer_.find('\n');
  }

  if (!polling_write_ && requests_.front().getStatus() == SENDING_RESPONSE) {
    ep_event_->events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
    action.op = EPOLL_ACTION_MOD;
    action.event = ep_event_;
    polling_write_ = true;
  }

  return action;
}

EpollAction Connection::handleWrite() {
  requests_.front().sendResponse();

  if (requests_.front().getStatus() == COMPLETED) {
    requests_.pop_front();
  }

  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, ep_event_};
  if (requests_.front().getStatus() != SENDING_RESPONSE) {
    ep_event_->events = EPOLLIN | EPOLLRDHUP;
    action.op = EPOLL_ACTION_MOD;
    polling_write_ = false;
  }

  return action;
}
