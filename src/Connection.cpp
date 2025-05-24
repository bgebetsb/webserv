#include "Connection.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <exception>
#include <string>
#include <vector>
#include "Configs/Configs.hpp"
#include "epoll/EpollAction.hpp"
#include "exceptions/ConError.hpp"
#include "exceptions/FdLimitReached.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/Request.hpp"
#include "requests/RequestStatus.hpp"
#include "responses/FileResponse.hpp"
#include "responses/StaticResponse.hpp"
#include "utils/Utils.hpp"

Connection::Connection(int socket_fd, const std::vector< Server >& servers)
    : servers_(servers),
      polling_write_(false),
      request_(Request(-1, servers)),
      request_timeout_ping_(Utils::getCurrentTime()),
      keepalive_last_ping_(0),
      send_receive_ping_(request_timeout_ping_)
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
  if (((event & EPOLLIN) | (event & EPOLLOUT)) != 0)
    send_receive_ping_ = Utils::getCurrentTime();
  if (event & EPOLLIN)
  {
    try
    {
      return handleRead();
    }
    catch (RequestError& e)
    {
      try
      {
        const Server& server = request_.getServer();
        MErrors::const_iterator it = server.error_pages.find(e.getCode());
        if (it == server.error_pages.end())
        {
          throw RequestError(404, "No error page configured");
        }
        const Location& location =
            Request::findMatchingLocationBlock(server.locations, it->second);
        if (!location.GET)
          throw RequestError(405, "Method not allowed for error page");
        if (location.root.empty())
          throw RequestError(404, "Error page: no root directory set");
        std::string path = location.root + '/' +
                           it->second.substr(location.location_name.length());
        request_.setResponse(new FileResponse(fd_, path, e.getCode(),
                                              request_.closingConnection()));
        ep_event_->events = EPOLLOUT;
        EpollAction action = {fd_, EPOLL_ACTION_MOD, getEvent()};
        return action;
      }
      catch (std::exception& e)
      {
        // Fall back to the Static-Response if something happens with the
        // ErrorPage
      }
      request_.setResponse(
          new StaticResponse(fd_, e.getCode(), request_.closingConnection()));
      ep_event_->events = EPOLLOUT;
      EpollAction action = {fd_, EPOLL_ACTION_MOD, getEvent()};
      return action;
    }
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
      throw RequestError(414, "Request URI too long");
    else if (request_.getStatus() == READING_HEADERS)
      throw RequestError(400, "Header too long");
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
    try
    {
      processBuffer();
    }
    catch (RequestError& e)
    {
      request_.setResponse(
          new StaticResponse(fd_, e.getCode(), request_.closingConnection()));
    }
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
  if ((request_timeout_ping_ > 0 &&
       current_time >=
           request_timeout_ping_ + REQUEST_TIMEOUT_SECONDS * 1000) ||
      current_time >= send_receive_ping_ + SEND_RECEIVE_TIMEOUT * 1000)
  {
    if (request_.getStatus() < SENDING_RESPONSE)
    {
      request_.timeout();
      action.event->events = EPOLLOUT | EPOLLRDHUP;
      action.op = EPOLL_ACTION_MOD;
      request_timeout_ping_ = 0;
    }
    else
    {
      action.op = EPOLL_ACTION_DEL;
    }
  }
  else
  {
    action.op = EPOLL_ACTION_UNCHANGED;
    if (keepalive_last_ping_ > 0)
      time_diff = current_time - keepalive_last_ping_;
  }

  return std::make_pair(action, time_diff);
}
