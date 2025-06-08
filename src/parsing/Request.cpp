#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Parsing.hpp"
#include "exceptions/RequestError.hpp"
#include "utils/Utils.hpp"

namespace Parsing
{
  using std::istringstream;
  using std::pair;
  using std::string;
  using std::vector;

  static void validateIpv6(const std::string& ipv6);
  static string escapeHostname(const std::string& hostname);
  static u_int16_t parsePort(const std::string& host);
  static void parseCookiePair(istringstream& stream);
  static void skipCookieOctets(istringstream& stream);
  static std::string getFieldContent(std::istringstream& stream);

  string processQueryString(const string query)
  {
    istringstream ss(query);
    string escaped;
    int c;

    while (true)
    {
      if (get_pchar(ss, c))
      {
        escaped += static_cast< char >(c);
        continue;
      }
      c = ss.get();
      if (ss.fail())
        break;
      if (c != '/' && c != '?')
        throw RequestError(400, "Invalid character in query section");
      escaped += static_cast< char >(c);
    }
    return escaped;
  }

  string processPath(const string path)
  {
    istringstream ss(path);
    vector< string > segments;
    int c;

    while (true)
    {
      c = ss.get();
      if (ss.fail())
        break;
      if (c != '/')
        throw RequestError(400,
                           "Unexpected character in processPath(expected /");
      string segment = get_segment(ss);
      if (segment.empty())
        continue;
      if (segment == "..")
      {
        if (!segments.empty())
          segments.pop_back();
      }
      else
        segments.push_back(segment);
    }

    string escaped;
    vector< string >::iterator it;
    for (it = segments.begin(); it != segments.end(); ++it)
      escaped += "/" + *it;

    if (escaped.empty() || path[path.length() - 1] == '/')
      escaped += '/';

    return escaped;
  }

  void validateUserinfo(const std::string& userinfo)
  {
    std::istringstream ss(userinfo);
    int c;

    while (true)
    {
      c = ss.get();
      if (ss.fail())
        break;
      if (c == '%')
        read_pct_encoded(ss);
      else if (is_unreserved(c) || is_sub_delims(c) || c == ':')
        ;
      else
        throw RequestError(400, "Invalid character in userinfo");
    }
  }

  pair< string, u_int16_t > parseHost(const string& host)
  {
    string::size_type pos;
    string hostname(host);
    string port_str;

    pos = hostname.find_last_of(':');
    if (pos != string::npos)
    {
      port_str = hostname.substr(pos);
      hostname = hostname.substr(0, pos);
    }

    if (hostname.empty())
      throw RequestError(400, "parse Host: empty hostname");
    if (hostname[0] == '[')
    {
      pos = hostname.find(']');
      if (pos == string::npos)
        throw RequestError(400, "parse host: closing bracket of ipv6 missing");
      if (pos != hostname.length() - 1)
        throw RequestError(
            400, "parse host: unexpected content after ipv6 closing bracket");
      validateIpv6(hostname.substr(1, pos - 1));
      hostname = hostname.substr(0, pos + 1);
    }
    else
      hostname = escapeHostname(hostname);

    u_int16_t port = parsePort(port_str);

    return std::make_pair(hostname, port);
  }

  static void validateIpv6(const std::string& ipv6)
  {
    struct addrinfo hints;
    struct addrinfo* result;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_flags = AI_NUMERICHOST;

    if (getaddrinfo(ipv6.c_str(), NULL, &hints, &result) != 0)
      throw RequestError(400, "validateIpv6: invalid format");
    else
      freeaddrinfo(result);
  }

  static string escapeHostname(const std::string& hostname)
  {
    istringstream ss(hostname);
    string escaped;
    int c;

    while (true)
    {
      c = ss.get();
      if (ss.fail())
        break;
      if (c == '%')
        escaped += read_pct_encoded(ss);
      else if (is_unreserved(c) || is_sub_delims(c))
        escaped += c;
      else
        throw RequestError(400, "Invalid character in userinfo");
    }

    return escaped;
  }

  static u_int16_t parsePort(const std::string& port_str)
  {
    std::istringstream ss(port_str);
    int c;
    u_int16_t port;

    c = ss.get();
    if (ss.fail())
      return (80);
    if (c != ':')
      throw RequestError(400, "parsePort: ':' not found");
    c = ss.get();
    if (ss.fail())
      return (80);
    if (!std::isdigit(c))
      throw RequestError(400, "parsePort: Non-numeric character after ':'");
    ss.unget();

    if (!(ss >> port))
      throw RequestError(400, "parsePort: Port outside the range");
    if (!ss.eof())
      throw RequestError(400, "parsePort: unexpected content after port");
    return port;
  }

  void validateCookies(const std::string& cookies)
  {
    int c;
    istringstream ss(cookies);

    parseCookiePair(ss);
    while (true)
    {
      c = ss.get();
      if (ss.fail())
        break;
      if (c != ';')
        throw RequestError(
            400, "validateCookies: Invalid character, expected semicolon");
      skip_character(ss, ' ');
      parseCookiePair(ss);
    }
  }

  static void parseCookiePair(istringstream& stream)
  {
    int c;

    skip_token(stream);
    c = stream.get();
    if (stream.fail())
      throw("parseCookiePair: EOF reached, expected =");
    if (c != '=')
      throw("parseCookiePair: Invalid character, expected =");
    c = stream.get();
    if (stream.fail())
      return;
    if (c == '"')
    {
      skipCookieOctets(stream);
      c = stream.get();
      if (stream.fail() || c != '"')
        throw("parseCookiePair: Unclosed double quote");
    }
    else
      skipCookieOctets(stream);
  }

  static void skipCookieOctets(istringstream& stream)
  {
    int c;

    while (true)
    {
      c = stream.get();
      if (stream.fail())
        break;
      if (c < 0x21 || c > 0x7E || c == 0x22 || c == 0x2C || c == 0x3B ||
          c == 0x5C)
      {
        stream.unget();
        break;
      }
    }
  }

  pair< string, string > parseFieldLine(const std::string& line)
  {
    std::istringstream ss(line);
    std::string name = Parsing::get_token(ss);
    Parsing::skip_character(ss, ':');
    Parsing::skip_ows(ss);
    std::string value = getFieldContent(ss);

    std::for_each(name.begin(), name.end(), Utils::toLower);
    return std::make_pair(name, value);
  }

  static std::string getFieldContent(std::istringstream& stream)
  {
    std::string content;
    std::string::size_type pos;
    int c;

    c = stream.get();
    if (stream.fail())
      throw RequestError(400, "Missing field content");

    do
    {
      if (!Parsing::is_vchar(c) && !Parsing::is_space(c))
        throw RequestError(400, "Invalid character in field value");

      content += c;
      c = stream.get();
    } while (stream.good());

    pos = content.find_last_not_of("\t ");
    if (pos != std::string::npos)
      content = content.substr(0, pos + 1);

    return content;
  }
}  // namespace Parsing
