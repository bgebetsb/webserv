#pragma once

#include <sys/types.h>
#include "FileTypes.hpp"

struct PathInfos
{
  bool exists;
  FileTypes types;
  bool readable;
  bool writable;
  bool executable;
  off_t size;
};
