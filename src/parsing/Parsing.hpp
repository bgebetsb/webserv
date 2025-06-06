#pragma once

#include <sstream>

namespace Parsing
{
  bool is_space(int c);
  bool is_qdtext(int c);
  bool is_obs_text(int c);
  bool is_vchar(int c);

  long read_hex(std::istringstream& stream);
  void skip_ows(std::istringstream& stream);
  void skip_token(std::istringstream& stream);

  size_t getChunkHeaderSize(const std::string& line);
}  // namespace Parsing
