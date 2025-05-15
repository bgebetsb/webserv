#include "Connection.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <vector>
#include "epoll/EpollAction.hpp"
#include "exceptions/ConError.hpp"
#include "exceptions/FdLimitReached.hpp"
#include "requests/Request.hpp"
#include "requests/RequestStatus.hpp"
#include "responses/StaticResponse.hpp"
#include "utils/Utils.hpp"

Connection::Connection(int socket_fd, const std::vector< Server >& servers)
    : servers_(servers),
      polling_write_(false),
      request_(Request(-1, servers)),
      request_timeout_ping_(Utils::getCurrentTime()),
      keepalive_last_ping_(0)
{
  struct sockaddr_in peer_addr;
  socklen_t peer_addr_size = sizeof(peer_addr);

  readbuf_ = new char[CHUNK_SIZE];
  fd_ = accept(socket_fd, (struct sockaddr*)&peer_addr, &peer_addr_size);
  if (fd_ == -1)
  {
    if (errno == EMFILE || errno == ENFILE)
      throw FdLimitReached("Unable to accept client connection");
    else
      throw ConErr("Unable to accept client connection");
  }

  if (fcntl(fd_, F_SETFL, O_NONBLOCK) == -1)
    throw ConErr("Unable to set fd to non-blocking");

  ep_event_->events = EPOLLIN | EPOLLRDHUP;

  ep_event_->data.ptr = this;
  request_ = Request(fd_, servers);
}

Connection::~Connection()
{
  delete[] readbuf_;
}

EpollAction Connection::epollCallback(int event)
{
  if (event & EPOLLIN)
  {
    return handleRead();
  }
  else if (event & EPOLLOUT)
  {
    return handleWrite();
  }

  EpollAction action = {fd_, EPOLL_ACTION_DEL, NULL};
  return action;
}

EpollAction Connection::handleRead()
{
  keepalive_last_ping_ = 0;
  ssize_t ret = recv(fd_, readbuf_, CHUNK_SIZE, 0);
  if (ret == -1)
    throw ConErr("Recv failed");
  else if (ret == 0)
    throw ConErr("Peer closed connection");
  buffer_.append(readbuf_, ret);

  return processBuffer();
}

EpollAction Connection::processBuffer()
{
  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, NULL};

  size_t pos = buffer_.find('\n');
  while (pos != std::string::npos)
  {
    size_t realpos = pos;
    if (realpos > 0 && buffer_[realpos - 1] == '\r')
    {
      --realpos;
    }
    std::string line(buffer_, 0, realpos);
    if (buffer_.size() > pos + 1)
    {
      buffer_ = std::string(buffer_, pos + 1);
    }
    else
    {
      buffer_.clear();
    }
    request_.addHeaderLine(line);

    if (request_.getStatus() == SENDING_RESPONSE)
    {
      request_timeout_ping_ = 0;
      break;
    }

    pos = buffer_.find('\n');
  }

  if (buffer_.size() > 8192)
  {
    if (request_.getStatus() == READING_START_LINE)
      request_.setResponse(new StaticResponse(fd_, 414));
    else if (request_.getStatus() == READING_HEADERS)
      request_.setResponse(new StaticResponse(fd_, 400));
  }

  if (!polling_write_ && request_.getStatus() == SENDING_RESPONSE)
  {
    ep_event_->events = EPOLLOUT | EPOLLRDHUP;
    action.op = EPOLL_ACTION_MOD;
    action.event = ep_event_;
    polling_write_ = true;
  }

  return action;
}

EpollAction Connection::handleWrite()
{
  bool closing = false;

  request_.sendResponse();

  if (request_.getStatus() == COMPLETED)
  {
    closing = request_.closingConnection();
    request_ = Request(fd_, servers_);
  }

  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, ep_event_};
  if (closing)
  {
    action.op = EPOLL_ACTION_DEL;
  }
  else if (request_.getStatus() != SENDING_RESPONSE)
  {
    ep_event_->events = EPOLLIN | EPOLLRDHUP;
    action.op = EPOLL_ACTION_MOD;
    polling_write_ = false;
    if (request_.getStatus() == READING_START_LINE)
    {
      keepalive_last_ping_ = Utils::getCurrentTime();
      request_timeout_ping_ = 0;
    }
    else if (request_.getStatus() == READING_HEADERS)
    {
      keepalive_last_ping_ = 0;
      request_timeout_ping_ = Utils::getCurrentTime();
    }
  }

  return action;
}

std::pair< EpollAction, u_int64_t > Connection::ping()
{
  EpollAction action;
  u_int64_t current_time;
  u_int64_t time_diff = 0;

  action.event = getEvent();
  action.fd = fd_;

  current_time = Utils::getCurrentTime();
  if (request_timeout_ping_ > 0 &&
      current_time >= request_timeout_ping_ + 30000)
  {
    request_.timeout();
    action.event->events = EPOLLOUT | EPOLLRDHUP;
    action.op = EPOLL_ACTION_MOD;
    request_timeout_ping_ = 0;
  }
  else
  {
    action.op = EPOLL_ACTION_UNCHANGED;
    if (keepalive_last_ping_ > 0)
      time_diff = current_time - keepalive_last_ping_;
  }

  return std::make_pair(action, time_diff);
}
