#include "Parsing.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <utility>
#include "../exceptions/RequestError.hpp"

namespace Parsing
{
  using std::istringstream;
  using std::string;

  static const std::pair< char, char > TCHAR_TOKEN_RANGES[] = {
      std::make_pair('#', '\''),
      std::make_pair('*', '+'),
      std::make_pair('-', '.'),
      std::make_pair('^', '`'),
  };
  static const char TCHAR_SOLO_TOKENS[] = {'!', '|', '~'};

  static const std::pair< char, char > SUB_DELIMS_RANGE =
      std::make_pair('&', ',');

  static const char SUB_DELIMS_SOLO_TOKENS[] = {'!', '$', ';', '='};

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

  long read_hex(istringstream& stream)
  {
    long number;

    stream >> std::hex >> number;
    if (stream.fail() && !stream.eof())
      throw RequestError(400, "Unable to convert to long");
    return number;
  }

  char read_pct_encoded(istringstream& stream)
  {
    int c, c2;
    char chars[3];
    long number;

    c = stream.get();
    c2 = stream.get();
    if (stream.fail())
      throw RequestError(400, "read_pct_encoded: Unable to get two characters");
    if (!isxdigit(c) || !isxdigit(c2))
      throw RequestError(400, "read_pct_encoded: non hex character");
    chars[0] = c;
    chars[1] = c2;
    chars[2] = '\0';

    number = std::strtol(chars, NULL, 16);
    return static_cast< char >(number);
  }

  void skip_hexadecimal(const string& input, string::size_type& pos)
  {
    if (pos >= input.length())
      throw RequestError(400, "Skip Hex: String too short");
    if (!std::isxdigit(input[pos]))
      throw RequestError(400, "Skip Hex: First character not hex");
    pos++;
    while (pos < input.length() && std::isxdigit(input[pos]))
      pos++;
  }

  void skip_ows(istringstream& stream)
  {
    int c;

    while (true)
    {
      c = stream.get();
      if (stream.fail())
        break;
      if (!is_space(c))
      {
        stream.unget();
        break;
      }
    }
  }

  void skip_token(istringstream& stream)
  {
    int c;

    c = stream.get();
    if (stream.fail())
      throw RequestError(400, "Skip Token: String too short");
    if (!is_tchar(c))
      throw RequestError(400, "Skip Token: First character not tchar");
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

  string get_token(istringstream& stream)
  {
    string token;
    int c;

    c = stream.get();
    if (stream.fail())
      throw RequestError(400, "get_token: String too short");
    if (!is_tchar(c))
      throw RequestError(400, "get_token: First character not tchar");
    token += c;
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
      token += c;
    }

    return token;
  }

  void skip_character(istringstream& stream, char expected)
  {
    int c;

    c = stream.get();
    if (stream.fail())
      throw RequestError(400, "skip_character: Already at EOF");
    if (c != expected)
      throw RequestError(400, "skip_character: Wrong character");
  }

  bool get_pchar(std::istringstream& stream, int& c)
  {
    int tmp;
    tmp = stream.get();
    if (stream.fail())
      return false;
    if (tmp == '%')
    {
      c = read_pct_encoded(stream);
      return true;
    }
    else if (is_unreserved(tmp) || is_sub_delims(tmp) || tmp == ':' ||
             tmp == '@')
    {
      c = tmp;
      return (true);
    }
    else
    {
      stream.unget();
      return (false);
    }
  }

  bool is_unreserved(int c)
  {
    if (std::isalnum(c))
      return (true);
    switch (c)
    {
      case '-':
      case '.':
      case '_':
      case '~':
        return true;
      default:
        return false;
    }
  }

  bool is_sub_delims(int c)
  {
    if (c >= SUB_DELIMS_RANGE.first && c <= SUB_DELIMS_RANGE.second)
      return true;
    unsigned int amount =
        sizeof(SUB_DELIMS_SOLO_TOKENS) / sizeof(SUB_DELIMS_SOLO_TOKENS[0]);

    for (unsigned int i = 0; i < amount; ++i)
    {
      if (c == SUB_DELIMS_SOLO_TOKENS[i])
        return true;
    }

    return false;
  }

  string get_segment(std::istringstream& stream)
  {
    string segment;
    int c;

    while (get_pchar(stream, c))
      segment += static_cast< char >(c);

    return segment;
  }

  void validateQuotedString(std::istringstream& stream)
  {
    int c;

    while (true)
    {
      c = stream.get();
      if (stream.fail())
        throw RequestError(400, "Unclosed quote");
      if (c == '"')
        return;
      if (is_qdtext(c))
        ;
      else if (c == '\\')
      {
        c = stream.get();
        if (stream.fail())
          throw RequestError(400, "EOF after Backslash");
        if (!is_space(c) && !is_vchar(c) && !is_obs_text(c))
          throw RequestError(400, "Invalid character in quoted string");
      }
      else
        throw RequestError(400, "Invalid character in quoted string");
    }
  }
}  // namespace Parsing
