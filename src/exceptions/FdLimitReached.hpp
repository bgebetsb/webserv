#pragma once

#include <exception>  // Required for inheriting from std::exception
#include <string>     // Required for std::string

#define BRIGHT_CYAN "\033[1;36m"
#define RESET "\033[0m"
#define FDLIMIT_PREFIX BRIGHT_CYAN "[Fd Limit Reached]: " RESET

class FdLimitReached : public std::exception
{
 public:
  explicit FdLimitReached(const std::string& msg) : msg_(FDLIMIT_PREFIX + msg)
  {}
  virtual ~FdLimitReached() throw() {}

  virtual const char* what() const throw()
  {
    return msg_.c_str();
  }

 private:
  std::string msg_;
};
