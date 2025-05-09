#pragma once

#include <string>
#include "PathInfos.hpp"

std::string preventEscaping(const std::string& path);
PathInfos getFileType(const std::string& filename);
