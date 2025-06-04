#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include "PathInfos.hpp"
#include "exceptions/RequestError.hpp"

static bool fileReadable(const std::string& filename);
static bool fileWritable(const std::string& filename);
static bool fileExecutable(const std::string& filename);

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
      infos.writable = false;
      infos.executable = false;
      infos.size = -1;
      return (infos);
    }
    else if (errno == ENOTDIR)
      return getFileType(filename.substr(0, filename.length() - 1));
    else if (errno == ENAMETOOLONG)
      throw RequestError(400, "Filename too long");
    throw RequestError(500, "Stat failed for unknown reason");
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
  infos.writable = fileWritable(filename);
  infos.executable = fileExecutable(filename);
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

static bool fileWritable(const std::string& filename)
{
  if (access(filename.c_str(), W_OK) == 0)
  {
    return (true);
  }
  return (false);
}

static bool fileExecutable(const std::string& filename)
{
  if (access(filename.c_str(), X_OK) == 0)
  {
    return (true);
  }
  return (false);
}
