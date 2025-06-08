#pragma once

#include <sys/types.h>
#include <sstream>

namespace Parsing
{
  bool is_space(int c);
  bool is_qdtext(int c);
  bool is_obs_text(int c);
  bool is_vchar(int c);
  bool get_pchar(std::istringstream& stream, int& c);
  bool is_unreserved(int c);
  bool is_sub_delims(int c);

  long read_hex(std::istringstream& stream);
  char read_pct_encoded(std::istringstream& stream);
  void skip_ows(std::istringstream& stream);
  void skip_token(std::istringstream& stream);
  void skip_character(std::istringstream& stream, char expected);

  size_t getChunkHeaderSize(const std::string& line);
  void validateChunkTrailer(const std::string& line);
  std::string get_token(std::istringstream& stream);
  std::string get_segment(std::istringstream& stream);

  std::pair< std::string, std::string > parseFieldLine(const std::string& line);
  std::string processQueryString(const std::string query);
  std::string processPath(const std::string path);
  std::pair< std::string, u_int16_t > parseHost(const std::string& host);
  void validateUserinfo(const std::string& userinfo);
  void validateCookies(const std::string& cookies);
}  // namespace Parsing
