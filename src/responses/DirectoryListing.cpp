#include "DirectoryListing.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>
#include "exceptions/RequestError.hpp"
#include "requests/PathValidation/FileTypes.hpp"
#include "utils/Utils.hpp"

namespace DirectoryListing
{
  static std::string buildDirectoryListing(const SEntries& entries,
                                           const std::string& request_path);
  static bool fillDirEntry(DirEntry& entry,
                           const std::string& local_path,
                           struct dirent* dir);

  static std::string escapeHTML(const std::string& original);
  static std::string applyURLEncoding(const std::string& original);

  std::string createDirectoryListing(const std::string& local_path,
                                     const std::string& request_path)
  {
    DIR* dirp = opendir(local_path.c_str());
    if (!dirp)
      throw RequestError(500, "Unable to open directory");

    struct dirent* dir;
    SEntries entries;

    while ((dir = readdir(dirp)))
    {
      DirEntry entry;

      entry.name = dir->d_name;
      if (entry.name == ".")
        continue;
      if (!fillDirEntry(entry, local_path, dir))
      {
        closedir(dirp);
        throw RequestError(500, "Filling directory entry failed");
      }
      switch (dir->d_type)
      {
        case DT_REG:
          entry.type = REGULAR_FILE;
          break;
        case DT_DIR:
          entry.type = DIRECTORY;
          entry.name += "/";
          break;
        default:
          entry.type = OTHER;
      }

      entries.insert(entry);
    }
    closedir(dirp);

    return buildDirectoryListing(entries, request_path);
  }

  static bool fillDirEntry(DirEntry& entry,
                           const std::string& local_path,
                           struct dirent* dir)
  {
    std::string full_path;
    struct stat st;

    full_path = local_path + "/" + dir->d_name;
    if (stat(full_path.c_str(), &st) == -1)
      return (false);

    entry.size = st.st_size;
    time_t modification_time = st.st_mtime;
    std::tm* time = std::localtime(&modification_time);
    if (!time)
      return (false);
    std::strftime(entry.timestamp, sizeof(entry.timestamp), "%Y-%m-%d %H:%M:%S",
                  time);
    return (true);
  }

  static std::string buildDirectoryListing(const SEntries& entries,
                                           const std::string& request_path)
  {
    std::ostringstream body_stream;
    body_stream << "<!DOCTYPE html>\r\n"
                   "<html>\r\n"
                   "<head>\r\n"
                   "<title>Index of " +
                       request_path +
                       "</title>\r\n"
                       "<style>\r\n"
                       "body{\r\n"
                       "padding-left:30px;\r\n"
                       "padding-right:30px;\r\n"
                       "table {\r\n"
                       "border-collapse: collapse;\r\n"
                       "width: 100%;\r\n"
                       "}\r\n"
                       "td {\r\n"
                       "border: none;\r\n"
                       "width: 33.3%;\r\n"
                       "}\r\n"
                       ".left {\r\n"
                       "text-align: left;\r\n"
                       "}\r\n"
                       ".center {\r\n"
                       "text-align: center;\r\n"
                       "}\r\n"
                       ".right {\r\n"
                       "text-align: right;\r\n"
                       "}\r\n"
                       "</style>\r\n"
                       "</head>\r\n"
                       "<body>\r\n"
                       "<h1>Index of " +
                       escapeHTML(request_path) +
                       "</h1>\r\n<table>\r\n<tr>\r\n<th "
                       "class=\"left\">Name</th>\r\n<th "
                       "class=\"center\">Last Modified</th>"
                       "<th class=\"right\">Size</th></tr>\r\n";
    SEntries::const_iterator it;

    for (it = entries.begin(); it != entries.end(); ++it)
    {
      body_stream << "<tr><td class=\"left\"><a href=\"" +
                         applyURLEncoding(it->name) + "\">" +
                         escapeHTML(it->name) +
                         "</a></td><td class=\"center\">";
      body_stream << it->timestamp;
      body_stream << "</td><td "
                     "class=\"right\">";
      if (it->type == REGULAR_FILE)
        body_stream << it->size;
      body_stream << "</td></tr>\r\n";
    }
    body_stream << "</table></body>\r\n</html>\r\n";

    return body_stream.str();
  }

  static const VRanges& getURLEncodingRanges()
  {
    static VRanges ranges;

    if (ranges.empty())
    {
      ranges.push_back(RangePair(0x20, 0x2C));
      ranges.push_back(RangePair(0x2F, 0x2F));
      ranges.push_back(RangePair(0x3A, 0x40));
      ranges.push_back(RangePair(0x5B, 0x5D));
      ranges.push_back(RangePair(0x7B, 0x7D));
    }

    return ranges;
  }

  static std::string escapeHTML(const std::string& original)
  {
    std::string current = Utils::replaceString(original, "&", "&amp;");
    current = Utils::replaceString(current, "<", "&lt;");
    current = Utils::replaceString(current, ">", "&gt;");
    current = Utils::replaceString(current, "\"", "&quot;");
    current = Utils::replaceString(current, "'", "&#39;");

    return current;
  }

  static std::string applyURLEncoding(const std::string& original)
  {
    const VRanges& ranges = getURLEncodingRanges();
    VRanges::const_iterator it;

    std::stringstream replaced;

    for (std::string::size_type i = 0; i < original.length(); ++i)
    {
      bool found = false;
      for (it = ranges.begin(); it != ranges.end(); ++it)
      {
        if (original[i] >= it->first && original[i] <= it->second)
        {
          replaced << "%" << std::uppercase << std::hex
                   << static_cast< u_int16_t >(original[i]);
          found = true;
          break;
        }
      }

      if (!found)
        replaced << original[i];
    }

    return replaced.str();
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
