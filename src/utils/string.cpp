#include <string>

namespace Utils {
  std::string trimString(const std::string& input) {
    if (input.empty()) {
      return (input);
    }

    std::string::size_type start = 0;
    std::string::size_type end = input.length();

    while (start < end && std::isspace(input[start])) {
      start++;
    }

    while (end > start && std::isspace(input[end - 1])) {
      end--;
    }

    return (input.substr(start, end - start));
  }
}  // namespace Utils
