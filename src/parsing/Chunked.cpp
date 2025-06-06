#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "Parsing.hpp"

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
      throw std::runtime_error("Negative chunk size");
    while (!ss.fail())
    {
      skip_ows(ss);
      if (ss.fail())
        break;
      c = ss.get();
      if (ss.fail() || c != ';')
        throw std::runtime_error("Semicolon not found after BWS");
      skip_ows(ss);
      skip_token(ss);
      skip_ows(ss);
      if (ss.fail())
        break;
      c = ss.get();
      if (ss.fail())
        throw std::runtime_error("= or ; not found");
      if (c == '=')
      {
        skip_ows(ss);
        c = ss.get();
        if (ss.fail())
          throw std::runtime_error("No character after equal sign");
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
        throw std::runtime_error("Unclosed quote");
      if (c == '"')
        return;
      if (is_qdtext(c))
        ;
      else if (c == '\\')
      {
        c = stream.get();
        if (stream.fail())
          throw std::runtime_error("EOF after Backslash");
        if (!is_space(c && !is_vchar(c && !is_obs_text(c))))
          throw std::runtime_error("Invalid character in quoted string");
      }
      else
        throw std::runtime_error("Invalid character in quoted string");
    }
  }
}  // namespace Parsing
