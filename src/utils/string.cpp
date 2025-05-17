#include <string>

namespace Utils
{
  std::string trimString(const std::string& input)
  {
    if (input.empty())
    {
      return (input);
    }

    std::string::size_type start = 0;
    std::string::size_type end = input.length();

    while (start < end && std::isspace(input[start]))
    {
      start++;
    }

    while (end > start && std::isspace(input[end - 1]))
    {
      end--;
    }

    return (input.substr(start, end - start));
  }

  size_t countSubstr(const std::string& str, const std::string& substr)
  {
    size_t count = 0;
    size_t pos = str.find(substr);
    while (pos != std::string::npos)
    {
      count++;
      pos = str.find(substr, pos + substr.length());
    }
    return count;
  }

  void toLower(char& c)
  {
    c = static_cast< char >(std::tolower(static_cast< unsigned char >(c)));
  }

  std::string replaceString(const std::string& input,
                            const std::string& search,
                            const std::string& replace)
  {
    std::string::size_type cur_pos = 0;
    std::string::size_type match_pos;
    std::string new_string;

    if (search.empty())
      return input;

    std::string::size_type orig_length = input.length();
    while (cur_pos < orig_length)
    {
      match_pos = input.find(search, cur_pos);
      if (match_pos == std::string::npos)
      {
        new_string.append(input.substr(cur_pos, orig_length - cur_pos));
        cur_pos += orig_length - cur_pos;
      }
      else
      {
        if (match_pos != cur_pos)
        {
          new_string.append(input.substr(cur_pos, match_pos - cur_pos));
          cur_pos += match_pos - cur_pos;
        }
        new_string.append(replace);
        cur_pos += search.length();
      }
    }
    return (new_string);
  }

}  // namespace Utils
