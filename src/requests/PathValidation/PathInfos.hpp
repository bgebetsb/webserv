#pragma once

#include <sys/types.h>
#include "FileTypes.hpp"

struct PathInfos
{
  bool exists;
  FileTypes types;
  bool readable;
  off_t size;
};
