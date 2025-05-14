#include <sstream>
#include <string>
#include <vector>

static std::string joinStrings(const std::vector< std::string >& parts,
                               char delimiter);

std::string preventEscaping(const std::string& path)
{
  std::string part;
  std::istringstream stream(path);
  std::vector< std::string > parts;

  while (std::getline(stream, part, '/'))
  {
    if (part.empty())
    {
      continue;
    }
    else if (part != "..")
    {
      parts.push_back(part);
    }
    else if (!parts.empty())
    {
      parts.pop_back();
    }
  }

  return (joinStrings(parts, '/'));
}

static std::string joinStrings(const std::vector< std::string >& parts,
                               char delimiter)
{
  std::string final;

  for (std::vector< std::string >::const_iterator it = parts.begin();
       it < parts.end(); ++it)
  {
    final += delimiter + *it;
  }

  return (final);
}
