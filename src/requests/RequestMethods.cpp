#include "RequestMethods.hpp"

std::string methodToString(const RequestMethod& method)
{
  switch (method)
  {
    case GET:
      return "GET";
    case POST:
      return "POST";
    case DELETE:
      return "DELETE";
    default:
      return "INVALID";
  }
}
