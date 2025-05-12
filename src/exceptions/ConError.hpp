#pragma once

#include <exception>  // Required for inheriting from std::exception
#include <string>     // Required for std::string

#define PURPLE "\033[35m"
#define RESET "\033[0m"
#define CONERR_PREFIX PURPLE "[Connection Error]: " RESET

class ConErr : public std::exception
{
 public:
  explicit ConErr(const std::string& msg) : msg_(CONERR_PREFIX + msg) {}
  virtual ~ConErr() throw() {}

  virtual const char* what() const throw()
  {
    return msg_.c_str();
  }

 private:
  std::string msg_;
};
