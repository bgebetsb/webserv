#pragma once

#include <string>

enum RequestMethod
{
  GET,
  POST,
  DELETE,
  INVALID
};

std::string methodToString(const RequestMethod& method);
