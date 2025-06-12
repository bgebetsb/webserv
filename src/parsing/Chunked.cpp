#include <sstream>
#include <string>
#include "../exceptions/RequestError.hpp"
#include "Parsing.hpp"

namespace Parsing
{
  size_t getChunkHeaderSize(const std::string& line)
  {
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

  // Just ignore return value since we most likely don't have a use for the
  // trailing section anyway
  void validateChunkTrailer(const std::string& line)
  {
    Parsing::parseFieldLine(line);
  }
}  // namespace Parsing
