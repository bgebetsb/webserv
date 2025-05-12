#pragma once

#include <exception>  // Required for inheriting from std::exception
#include <string>     // Required for std::string

#define RED "\033[31m"
#define RESET "\033[0m"
#define FATAL_PREFIX RED "[Fatal]: " RESET

class Fatal : public std::exception
{
 public:
  explicit Fatal(const std::string& message) : message_(FATAL_PREFIX + message)
  {}
  virtual ~Fatal() throw() {}

  virtual const char* what() const throw()
  {
    return message_.c_str();
  }

 private:
  std::string message_;
};
