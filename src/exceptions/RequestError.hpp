#pragma once

#include <exception>  // Required for inheriting from std::exception
#include <string>     // Required for std::string

#define REQUESTERROR_PREFIX "[Request Error]: "

class RequestError : public std::exception
{
 public:
  explicit RequestError(int code, const std::string& msg)
      : code_(code), msg_(REQUESTERROR_PREFIX + msg)
  {}
  virtual ~RequestError() throw() {}

  virtual const char* what() const throw()
  {
    return msg_.c_str();
  }

  int getCode() const
  {
    return code_;
  }

 private:
  int code_;
  std::string msg_;
};
