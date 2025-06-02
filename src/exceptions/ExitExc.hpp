#pragma once

#include <exception>  // Required for inheriting from std::exception
#include <string>     // Required for std::string

#define EXITERR 69

class ExitExc : public std::exception
{
 public:
  ExitExc() {}
  virtual ~ExitExc() throw() {}

  virtual const char* what() const throw()
  {
    return msg_.c_str();
  }

 private:
  std::string msg_;
};
