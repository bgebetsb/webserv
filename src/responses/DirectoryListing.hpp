#pragma once

#include <dirent.h>
#include <sys/types.h>
#include <set>
#include <string>
#include <vector>
#include "requests/PathValidation/FileTypes.hpp"

namespace DirectoryListing
{

  typedef std::pair< u_int8_t, u_int8_t > RangePair;
  typedef std::vector< RangePair > VRanges;

  struct DirEntry
  {
    FileTypes type;
    std::string name;
    off_t size;
    char timestamp[20];
  };

  typedef std::set< DirEntry > SEntries;

  std::string createDirectoryListing(const std::string& local_path,
                                     const std::string& request_path);
  bool operator<(const DirEntry& a, const DirEntry& b);
}  // namespace DirectoryListing
