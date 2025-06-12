#include "EpollData.hpp"
#include "../exceptions/Fatal.hpp"
#include "../utils/Utils.hpp"

EpollData::EpollData() : fd(epoll_create(1024))
{
  if (fd == -1)
    throw Fatal("epoll_create failed");

  if (Utils::addCloExecFlag(fd) == -1)
  {
    close(fd);
    throw Fatal("Unable to set O_CLOEXEC to epoll fd");
  }
}

EpollData::~EpollData()
{
  typedef std::map< const int, EpollFd* >::iterator iter_type;

  for (iter_type it = fds.begin(); it != fds.end(); ++it)
    delete it->second;

  close(fd);
}

EpollData& getEpollData()
{
  static EpollData ed;
  return ed;
}
