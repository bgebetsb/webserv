#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Utils.hpp"

int Utils::addCloExecFlag(int fd)
{
  int old_flags = fcntl(fd, F_GETFD);

  if (old_flags == -1)
    return -1;

  if (fcntl(fd, F_SETFD, old_flags | FD_CLOEXEC) == -1)
    return -1;
  return 0;
}

void Utils::ft_close(int& fd)
{
  if (fd != -1)
  {
    close(fd);
    fd = -1;
  }
}
