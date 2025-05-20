#include "DirectoryListing.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include "exceptions/RequestError.hpp"
#include "requests/PathValidation/FileTypes.hpp"

namespace DirectoryListing
{
  static std::string buildDirectoryListing(const SEntries& entries,
                                           const std::string& request_path);

  std::string createDirectoryListing(const std::string& local_path,
                                     const std::string& request_path)
  {
    DIR* dirp = opendir(local_path.c_str());
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
          full_path = local_path + "/" + dir->d_name;
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

    return buildDirectoryListing(entries, request_path);
  }

  static std::string buildDirectoryListing(const SEntries& entries,
                                           const std::string& request_path)
  {
    std::string response_body = "<html>\r\n"
                                "<head>\r\n"
                                "<title>Index</title>\r\n"
                                "</head>\r\n"
                                "<body>\r\n"
                                "<h1>Index of " +
                                request_path + "</h1>\r\n<br>\r\n";
    SEntries::const_iterator it;

    for (it = entries.begin(); it != entries.end(); ++it)
    {
      response_body +=
          "<a href=\"" + it->name + "\">" + it->name + "</a><br />\r\n";
    }
    response_body += "</body>\r\n</html>\r\n";

    return response_body;
  }

  bool operator<(const DirEntry& a, const DirEntry& b)
  {
    if (a.type == DIRECTORY && b.type != DIRECTORY)
      return true;
    else if ((a.type != DIRECTORY && b.type == DIRECTORY) || b.name == "..")
      return false;

    return (a.name < b.name);
  }

}  // namespace DirectoryListing
