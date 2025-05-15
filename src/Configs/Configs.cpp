#include "Configs.hpp"
#include <sys/types.h>

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include "../exceptions/Fatal.hpp"
#include "../ip/IpAddress.hpp"
#include "../ip/Ipv4Address.hpp"
#include "../ip/Ipv6Address.hpp"
#include "../utils/Utils.hpp"
#include "configUtils.hpp"

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Con. / Destructors     ║
// ╚══════════════════════════════════════════════╝

Configuration::Configuration(const string& config_file)
    : config_file_(config_file)

{
  (void)config_file;
  cgi_timeout_ = std::make_pair(CGI_TIMEOUT_MAX, false);
  keep_alive_timeout_ = std::make_pair(KEEP_ALIVE_TIMEOUT_MAX, false);
}

// Default constructor only enabled for testing purposes
Configuration::Configuration()
{
  cgi_timeout_ = std::make_pair(0, false);
  keep_alive_timeout_ = std::make_pair(0, false);
}

Configuration::~Configuration()
{
  for (size_t i = 0; i < server_configs_.size(); ++i)
  {
    for (IpSet::iterator it = server_configs_[i].ips.begin();
         it != server_configs_[i].ips.end(); ++it)
      delete *it;
  }
}

// ╔══════════════════════════════════════════════╗
// ║              SECTION: File Parsing           ║
// ╚══════════════════════════════════════════════╝

string::size_type findClosingBracket(const string& line, string::size_type pos)
{
  int bracket_count = 1;
  for (string::size_type i = pos + 1; i < line.size(); ++i)
  {
    if (line[i] == '{')
      ++bracket_count;
    else if (line[i] == '}')
      --bracket_count;
    if (bracket_count == 0)
      return i;
  }
  throw Fatal("Invalid config file format: expected '}'");
}

void Configuration::parseConfigFile(const string& config_file)
{
  (void)config_file;
  std::ifstream file(config_file.c_str());
  if (!file.is_open())
    throw Fatal("Could not open config file");

  string::size_type cursor = 0;
  std::stringstream file_buffer;
  file_buffer << file.rdbuf();
  std::string file_str = file_buffer.str();
  while (1)
  {
    std::string::size_type pos = file_str.find_first_of("{;", cursor);
    if (pos == std::string::npos)
      break;
    std::string identifier = file_str.substr(cursor, pos - cursor);
    if (file_str[pos] == '{')
    {
      std::stringstream ss(identifier);
      std::string identifier_token;
      ss >> identifier_token;
      if (identifier_token != "server")
        throw Fatal("Invalid config file format: expected 'server'");
      if (identifier_token == "server" && ss >> identifier_token)
        throw Fatal("Invalid config file format: invalid token count");
      string::size_type end = findClosingBracket(file_str, pos);
      std::string block = file_str.substr(pos + 1, end - pos + 1);
      process_server_block(block);
      cursor = end + 1;
    }
    else if (file_str[pos] == ';')
    {
      std::stringstream ss(identifier);
      std::string identifier_token;
      ss >> identifier_token;
      if (identifier_token == "cgi_timeout")
      {
        if (cgi_timeout_.second)
          throw Fatal(
              "Invalid config file format: cgi_timeout already defined");
        std::string token;
        if (!(ss >> token))
          throw Fatal("Invalid config file format: expected cgi_timeout value");
        if (token.find_first_not_of("0123456789") != string::npos)
          throw Fatal(
              "Invalid config file format: invalid cgi_timeout value => " +
              token);
        try
        {
          cgi_timeout_.first = Utils::ipStrToUint32Max(token, CGI_TIMEOUT_MAX);
        }
        catch (const Fatal& e)
        {
          throw Fatal(
              "Invalid config file format: invalid cgi_timeout value => " +
              token);
        }
        cgi_timeout_.second = true;
      }
      else if (identifier_token == "keep_alive_timeout")
      {
        if (keep_alive_timeout_.second)
          throw Fatal(
              "Invalid config file format: keep_alive_timeout already defined");
        std::string token;
        if (!(ss >> token))
          throw Fatal("Invalid config file format: expected "
                      "keep_alive_timeout value");
        if (token.find_first_not_of("0123456789") != string::npos)
          throw Fatal("Invalid config file format: invalid "
                      "keep_alive_timeout value => " +
                      token);
        try
        {
          keep_alive_timeout_.first =
              Utils::ipStrToUint32Max(token, KEEP_ALIVE_TIMEOUT_MAX);
        }
        catch (const Fatal& e)
        {
          throw Fatal("Invalid config file format: invalid keep_alive_timeout "
                      "value => " +
                      token);
        }
        keep_alive_timeout_.second = true;
      }
      else
      {
        throw Fatal("Invalid config file format: unknown global token => " +
                    identifier_token);
      }
      cursor = pos + 1;  // weiter hinter dem Semikolon
    }
  }
}

void Configuration::process_server_block(const std::string& block)
{
  serv_config config;
  std::size_t cursor = 0;

  // std::cout << block << std::endl;
  while (cursor < block.size())
  {
    std::size_t pos = block.find_first_of(";{", cursor);
    if (pos == std::string::npos)
      break;
    if (block[pos] == ';')
    {
      std::stringstream ss(block.substr(cursor, pos - cursor));
      process_server_item(ss, config);
    }
    else if (line[pos] == '{')
    {}
  }
}

static void insert_ip(IpSet& ips, const string& token)
{
  if (token.find_first_of(".:[]") == string::npos)
  {
    u_int16_t port = Utils::ipStrToUint16(token);
    if (port == 0)
      throw Fatal("Invalid config file format: port cannot be 0");
    Ipv4Address* addr = new Ipv4Address(0, port);
    if (!ips.insert(addr).second)
    {
      delete addr;
      throw Fatal("Invalid config file format: duplicate IP address");
    }
  }
  else if (token.find("[::]:") != string::npos)
  {
    string::size_type pos = token.find("[::]:");
    if (pos != 0)
      throw Fatal("Invalid config file format: [::]: needs to be at the "
                  "beginning");
    std::string port = token.substr(pos + 5);
    u_int16_t port_num = Utils::ipStrToUint16(port);
    if (port_num == 0)
      throw Fatal("Invalid config file format: port cannot be 0");
    Ipv6Address* addr = new Ipv6Address(port_num);
    if (!ips.insert(addr).second)
    {
      delete addr;
      throw Fatal("Invalid config file format: duplicate IP address");
    }
  }
  else if (token[0] == '[')
  {
    Ipv6Address* addr = new Ipv6Address(token);
    if (!ips.insert(addr).second)
    {
      delete addr;
      throw Fatal("Invalid config file format: duplicate IP address");
    }
  }
  else
  {
    Ipv4Address* addr = new Ipv4Address(token);
    if (!ips.insert(addr).second)
    {
      delete addr;
      throw Fatal("Invalid config file format: duplicate IP address");
    }
  }
}

void Configuration::process_server_item(std::stringstream& item,
                                        serv_config& config)
{
  (void)item;
  std::string token;
  item >> token;

  // ── ◼︎ server_name ──────────────────────────────────────────────────────────
  if (token == "server_name")  // needs aditional checks afterwards
  {
    std::vector< string > tokens;
    while (item >> token)
      tokens.push_back(token);
    if (tokens.size() == 0)
      throw Fatal("Invalid config file format: server_name requires at least 1 "
                  "argument");
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      if (Utils::isValidHostname(tokens[i]))
      {
        if (config.server_names.insert(tokens[i]).second == false)
        {
          std::cerr << WARNING << "Duplicate server name, statement ignored => "
                    << tokens[i] << std::endl;
          continue;
        }
      }
      else
        throw Fatal("Invalid config file format: invalid server name => " +
                    tokens[i]);
    }
  }
  // ── ◼︎ listen ─────────────────────────────────────────────────────────────
  else if (token == "listen")
  {
    if (!(item >> token))
      throw Fatal("Invalid config file format: expected ip address");
    insert_ip(config.ips, token);
    if (item >> token)
      throw Fatal("Invalid config file format: Token after ip address found");
  }
}

#include <iostream>
int main()
{
  std::string test = "test 123 hallo;";
  string::size_type pos = test.find_first_of(";{");

  if (pos != string::npos)
  {}
  else
  {
    std::cout << "Not found" << std::endl;
  }
}
