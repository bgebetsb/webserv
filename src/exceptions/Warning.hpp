#pragma once

#include <exception>  // Required for inheriting from std::exception
#include <string>     // Required for std::string

#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define WARNING_PREFIX YELLOW "[Warning]: " RESET

class Warning : public std::exception
{
 public:
  explicit Warning(const std::string& message)
      : message_(WARNING_PREFIX + message)
  {}
  virtual ~Warning() throw() {}

  virtual const char* what() const throw()
  {
    return message_.c_str();
  }

 private:
  std::string message_;
};
