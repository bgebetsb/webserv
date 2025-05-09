#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include "PathInfos.hpp"

static bool fileReadable(const std::string& filename);

PathInfos getFileType(const std::string& filename)
{
  struct stat statbuf;
  PathInfos infos;

  if (stat(filename.c_str(), &statbuf) == -1)
  {
    if (errno == ENOENT)
    {
      infos.exists = false;
      infos.types = OTHER;
      infos.readable = false;
      return (infos);
    }
    throw std::runtime_error("Stat failed");
  }

  infos.exists = true;
  switch (statbuf.st_mode & S_IFMT)
  {
    case S_IFDIR:
      infos.types = DIRECTORY;
      break;
    case S_IFREG:
      infos.types = REGULAR_FILE;
      break;
    default:
      infos.types = OTHER;
  }

  infos.readable = fileReadable(filename);
  return (infos);
}

static bool fileReadable(const std::string& filename)
{
  if (access(filename.c_str(), R_OK) == 0)
  {
    return (true);
  }

  return (false);
}
