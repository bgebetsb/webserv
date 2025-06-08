#include <iostream>
#include <sstream>
#include <string>
#include "Parsing.hpp"
#include "exceptions/RequestError.hpp"

namespace Parsing
{
  static inline void __attribute__((always_inline)) validateQuotedString(
      std::istringstream& string);

  size_t getChunkHeaderSize(const std::string& line)
  {
    std::cout << "Line: '" << line << "'\n";
    std::istringstream ss(line);
    int c;

    long chunk_size = read_hex(ss);
    if (chunk_size < 0)
      throw RequestError(400, "Negative chunk size");
    while (!ss.fail())
    {
      skip_ows(ss);
      if (ss.fail())
        break;
      c = ss.get();
      if (ss.fail() || c != ';')
        throw RequestError(400, "Semicolon not found after BWS");
      skip_ows(ss);
      skip_token(ss);
      skip_ows(ss);
      if (ss.fail())
        break;
      c = ss.get();
      if (ss.fail())
        throw RequestError(400, "= or ; not found");
      if (c == '=')
      {
        skip_ows(ss);
        c = ss.get();
        if (ss.fail())
          throw RequestError(400, "No character after equal sign");
        if (c == '"')
          validateQuotedString(ss);
        else
        {
          ss.unget();
          skip_token(ss);
        }
      }
      else
      {
        ss.unget();
        ss.unget();
      }
    }

    return static_cast< size_t >(chunk_size);
  }

  static inline void __attribute__((always_inline)) validateQuotedString(
      std::istringstream& stream)
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
        if (!is_space(c && !is_vchar(c && !is_obs_text(c))))
          throw RequestError(400, "Invalid character in quoted string");
      }
      else
        throw RequestError(400, "Invalid character in quoted string");
    }
  }

  // Just ignore return value since we most likely don't have a use for the
  // trailing section anyway
  void validateChunkTrailer(const std::string& line)
  {
    Parsing::parseFieldLine(line);
  }
}  // namespace Parsing
