#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include "PathInfos.hpp"
#include "exceptions/ConError.hpp"

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
      infos.size = -1;
      return (infos);
    }
    throw ConErr("Stat failed");
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
  infos.size = statbuf.st_size;
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
