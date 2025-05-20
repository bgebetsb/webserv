#pragma once

#include <dirent.h>
#include <sys/types.h>
#include <set>
#include <string>
#include "requests/PathValidation/FileTypes.hpp"

namespace DirectoryListing
{

  struct DirEntry
  {
    FileTypes type;
    std::string name;
    off_t size;
  };

  typedef std::set< DirEntry > SEntries;

  std::string createDirectoryListing(const std::string& local_path,
                                     const std::string& request_path);
  bool operator<(const DirEntry& a, const DirEntry& b);
}  // namespace DirectoryListing
