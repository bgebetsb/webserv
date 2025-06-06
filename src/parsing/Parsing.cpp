#include "Parsing.hpp"
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Parsing
{
  static const std::pair< char, char > TCHAR_TOKEN_RANGES[] = {
      std::make_pair('#', '\''),
      std::make_pair('*', '+'),
      std::make_pair('-', '.'),
      std::make_pair('^', '`'),
  };
  static const char TCHAR_SOLO_TOKENS[] = {'!', '|', '~'};

  static inline bool __attribute__((always_inline)) is_tchar(int c);

  static inline bool __attribute__((always_inline)) is_tchar(int c)
  {
    if (std::isalnum(c))
      return true;

    unsigned int token_ranges_count =
        sizeof(TCHAR_TOKEN_RANGES) / sizeof(TCHAR_TOKEN_RANGES[0]);
    for (unsigned int i = 0; i < token_ranges_count; ++i)
    {
      if (c >= TCHAR_TOKEN_RANGES[i].first && c <= TCHAR_TOKEN_RANGES[i].second)
        return true;
    }

    unsigned int solo_tokens_count =
        sizeof(TCHAR_SOLO_TOKENS) / sizeof(TCHAR_SOLO_TOKENS[0]);
    for (unsigned int i = 0; i < solo_tokens_count; ++i)
    {
      if (c == TCHAR_SOLO_TOKENS[i])
        return true;
    }

    return false;
  }

  bool is_space(int c)
  {
    return c == ' ' || c == '\t';
  }

  bool is_qdtext(int c)
  {
    return is_space(c) || c == 0x21 || (c >= 0x23 && c <= 0x5B) ||
           (c >= 0x5D && c <= 0x7E) || is_obs_text(c);
  }

  bool is_obs_text(int c)
  {
    return c >= 0x80 && c <= 0xFF;
  }

  bool is_vchar(int c)
  {
    return c >= 0x21 && c <= 0x7E;
  }

  long read_hex(std::istringstream& stream)
  {
    long number;

    stream >> std::hex >> number;
    if (stream.fail() && !stream.eof())
      throw std::runtime_error("Unable to convert to long");
    return number;
  }

  void skip_hexadecimal(const std::string& input, std::string::size_type& pos)
  {
    if (pos >= input.length())
      throw std::runtime_error("Skip Hex: String too short");
    if (!std::isxdigit(input[pos]))
      throw std::runtime_error("Skip Hex: First character not hex");
    pos++;
    while (pos < input.length() && std::isxdigit(input[pos]))
      pos++;
  }

  void skip_ows(std::istringstream& stream)
  {
    int c;

    c = stream.get();
    if (stream.good() && !is_space(c))
      stream.unget();
  }

  void skip_token(std::istringstream& stream)
  {
    int c;

    c = stream.get();
    if (stream.fail())
      throw std::runtime_error("Skip Token: String too short");
    if (!is_tchar(c))
      throw std::runtime_error("Skip Token: First character not tchar");
    while (true)
    {
      c = stream.get();
      if (stream.fail())
        break;
      if (!is_tchar(c))
      {
        stream.unget();
        break;
      }
    }
  }
}  // namespace Parsing
