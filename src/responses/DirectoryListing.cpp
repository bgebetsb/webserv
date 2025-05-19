#include <dirent.h>
#include <sys/stat.h>
#include <set>
#include "exceptions/RequestError.hpp"
#include "requests/PathValidation/FileTypes.hpp"

struct DirEntry
{
  FileTypes type;
  std::string name;
  off_t size;
};

bool operator<(const DirEntry& a, const DirEntry& b)
{
  if (a.type == DIRECTORY && b.type != DIRECTORY)
    return true;
  else if ((a.type != DIRECTORY && b.type == DIRECTORY) || b.name == "..")
    return false;

  return (a.name < b.name);
}

namespace DirectoryListing
{
  typedef std::set< DirEntry > SEntries;
  static std::string buildDirectoryListing(const SEntries& entries);

  std::string createDirectoryListing(const std::string& path)
  {
    DIR* dirp = opendir(path.c_str());
    if (!dirp)
      throw RequestError(500, "Unable to open directory");

    struct dirent* dir;
    struct stat st;
    SEntries entries;
    std::string full_path;

    while ((dir = readdir(dirp)))
    {
      DirEntry entry;

      entry.name = dir->d_name;
      if (entry.name == ".")
        continue;
      switch (dir->d_type)
      {
        case DT_REG:
          entry.type = REGULAR_FILE;
          full_path = path + "/" + dir->d_name;
          if (stat(full_path.c_str(), &st) == -1)
          {
            closedir(dirp);
            throw RequestError(500, "Stat failed (dirlisting)");
          }
          entry.size = st.st_size;
          break;
        case DT_DIR:
          entry.type = DIRECTORY;
          entry.name += "/";
          entry.size = 0;
          break;
        default:
          entry.size = 0;
          entry.type = OTHER;
      }

      entries.insert(entry);
    }
    closedir(dirp);

    return buildDirectoryListing(entries);
  }

  static std::string buildDirectoryListing(const SEntries& entries)
  {
    std::string response_body =
        "<html><head><title>Index</title></head><body><h1>Most beautiful "
        "directory listing ever</h1><br>";
    SEntries::const_iterator it;

    for (it = entries.begin(); it != entries.end(); ++it)
    {
      response_body +=
          "<a href=\"" + it->name + "\">" + it->name + "</a><br />\r\n";
    }
    response_body += "</body></html>";

    return response_body;
  }
}  // namespace DirectoryListing
