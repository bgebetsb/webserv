#include "Configs.hpp"
#include <sys/types.h>

#include <sys/stat.h>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
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
  checkFileType(config_file);

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

void Configuration::checkFileType(const std::string& filename) const
{
  struct stat st;

  if (stat(filename.c_str(), &st) != 0)
    throw Fatal("Error opening " + filename + ": " + strerror(errno));

  if (!S_ISREG(st.st_mode))
    throw Fatal("Requested config " + filename + " is not a regular file");
}

void Configuration::process_server_block(const std::string& block)
{
  Server config;
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
      cursor = pos + 1;  // weiter hinter dem Semikolon
    }
    else if (block[pos] == '{')
    {
      std::size_t end = block.find('}', pos);
      if (end == std::string::npos)
        throw Fatal("Invalid config file format: expected '}'");

      std::stringstream ss(block.substr(cursor, end - cursor + 1));
      process_location_block(ss, config);
      cursor = end + 1;  // weiter hinter der schließenden Klammer
    }
  }
  // TODO: check here if configs are valid
  server_configs_.push_back(config);
}

void Configuration::process_location_block(std::stringstream& item,
                                           Server& config)
{
  std::string line;
  std::getline(item, line, '{');
  if (line.empty())
    throw Fatal("Invalid config file format: expected location");
  std::stringstream ss(line);
  std::string token;
  ss >> token;

  if (token != "location")
    throw Fatal("Invalid config file format: expected 'location'");

  if (!(ss >> token) || token[0] != '/')
    throw Fatal("Invalid config file format: expected location path");

  if (config.locations.find(token) != config.locations.end())
    throw Fatal("Invalid config file format: duplicate location path");

  location new_location;
  if (config.locations.find(token) != config.locations.end())
    throw Fatal("Invalid config file format: duplicate location path");
  config.locations[token] = new_location;
  std::string location_path = token;
  if (line.empty())
    throw Fatal("Ivalid config file format: empty location block");
  while (std::getline(item, token, ';'))
  {
    if (token.empty() || token[0] == '}')
      break;
    std::stringstream ss3(token);
    process_location_item(ss3, config.locations[location_path]);
  }
}

void Configuration::process_location_item(std::stringstream& item,
                                          location& loc)
{
  std::string identifier;
  std::string token;
  item >> identifier;
  std::vector< string > tokens;
  while (item >> token)
  {
    tokens.push_back(token);
  }
  // ── ◼︎ http_methods  ──────────────────────────────────────────────────────
  if (identifier == "http_methods")
  {
    if (tokens.size() == 0)
      throw Fatal(
          "Invalid config file format: http_methods requires at least 1 "
          "argument");
    if (loc.http_methods_set)
      throw Fatal("Invalid config file format: http_methods already defined");
    loc.http_methods_set = true;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      if (tokens[i] == "GET")
        loc.GET = true;
      else if (tokens[i] == "POST")
        loc.POST = true;
      else if (tokens[i] == "DELETE")
        loc.DELETE = true;
      else
        throw Fatal("Invalid config file format: invalid http method => " +
                    tokens[i]);
    }
  }

  // ── ◼︎ root ───────────────────────────────────────────────────────────────
  else if (identifier == "root")
  {
    if (!loc.root.empty())
      throw Fatal("Invalid config file format: root already defined");
    if (tokens.size() != 1)
      throw Fatal("Invalid config file format: root requires exactly 1 "
                  "argument");
    if (tokens.back()[0] != '/')
    {
      throw Fatal(
          "Invalid config file format: root requires an absolute path => " +
          tokens[0]);
    }
    loc.root = tokens[0];
  }

  // ── ◼︎ index ──────────────────────────────────────────────────────────────
  else if (identifier == "index")
  {
    if (tokens.size() == 0)
      throw Fatal("Invalid config file format: index requires at least 1 "
                  "argument");
    for (size_t i = 0; i < tokens.size(); ++i)
      loc.default_files.push_back(tokens[i]);
  }

  // ── ◼︎ cgi ────────────────────────────────────────────────────────────────
  else if (identifier == "cgi")
  {
    if (tokens.empty())
      throw Fatal("Invalid config file format: cgi requires at least 1 "
                  "argument");
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      if (!is_valid_cgi(tokens[i]))
        throw Fatal("Invalid config file format: invalid cgi path => " +
                    tokens[i]);
      if (loc.cgi_extensions.find(tokens[i]) != loc.cgi_extensions.end())
      {
        std::cerr << WARNING << "Duplicate cgi extension, statement ignored => "
                  << tokens[i] << std::endl;
        continue;
      }
      loc.cgi_extensions.insert(tokens[i]);
    }
  }

  // ── ◼︎ DIR_LISTENING ──────────────────────────────────────────────────────
  else if (identifier == "autoindex")
  {
    if (tokens.size() != 1)
      throw Fatal("Invalid config file format: autoindex requires exactly 1 "
                  "argument");
    if (loc.DIR_LISTING.second)
      throw Fatal("Invalid config file format: autoindex already defined");
    if (tokens[0] == "on")
      loc.DIR_LISTING.first = true;
    else if (tokens[0] == "off")
      loc.DIR_LISTING.first = false;
    else
      throw Fatal("Invalid config file format: invalid autoindex value => " +
                  tokens[0]);
    loc.DIR_LISTING.second = true;
  }

  // ── ◼︎ client max body size ───────────────────────────────────────────────
  else if (identifier == "client_max_body_size")
  {
    if (tokens.size() != 1)
      throw Fatal("Invalid config file format: client_max_body_size requires "
                  "exactly 1 argument");
    if (loc.max_body_size.second)
      throw Fatal("Invalid config file format: client_max_body_size already "
                  "defined");
    loc.max_body_size.first = Utils::strToMaxBodySize(tokens[0]);
    loc.max_body_size.second = true;
  }

  // ── ◼︎ redirections ───────────────────────────────────────────────────────
  else if (identifier == "return")
  {
    if (tokens.empty() || tokens.size() > 2)
      throw Fatal(
          "Invalid config file format: return requires 1 or 2 arguments");
    if (loc.redirect.has_been_set)
      throw Fatal("Invalid config file format: return already defined");

    if (tokens[0].find_first_not_of("0123456789") != string::npos)
      throw Fatal("Invalid config file format: invalid return code => " +
                  tokens[0]);
    u_int16_t code = Utils::errStrToUint16(tokens[0]);
    if (found_code(code) && tokens.size() == 2)
      throw Fatal("Invalid config file format: return code => " + tokens[0] +
                  " should not have a URI");
    else if (found_code(code) && tokens.size() == 1)
    {
      loc.redirect.code = code;
      loc.redirect.has_been_set = true;
    }
    else if (is_valid_redirection_code(code) && tokens.size() == 1)
      throw Fatal("Invalid config file format: return code => " + tokens[0] +
                  " should have a URI");
    else if (is_valid_redirection_code(code) && tokens.size() == 2)
    {
      loc.redirect.code = code;
      if (!Utils::isValidUri(tokens[1]))
        throw Fatal("Invalid config file format: return URI => " + tokens[1] +
                    " should start with '/'");
      loc.redirect.uri = tokens[1];
      loc.redirect.has_been_set = true;
    }
  }

  // ── ◼︎ upload directory ───────────────────────────────────────────────────
  else if (identifier == "upload_dir")
  {
    if (loc.upload_dir.empty() == false)
      throw Fatal("Invalid config file format: upload_dir already defined");
    if (tokens.size() != 1)
      throw Fatal("Invalid config file format: upload_dir requires exactly 1 "
                  "argument");
    if (tokens[0][0] != '/')
      throw Fatal("Invalid config file format: upload_dir requires an "
                  "absolute path");
    loc.upload_dir = tokens[0];
  }

  // ── ◼︎ invalid token ────────────────────────────────────────────────────────
  else if (identifier == "}")
    return;
  else
    throw Fatal(
        "Invalid config file format: unknown token in location block => " +
        identifier);
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

void Configuration::process_server_item(std::stringstream& item, Server& config)
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
  // ── ◼︎ error page ────────────────────────────────────────────────────────
  else if (token == "error_page")  // dup ok,  but warning
  {
    std::vector< string > tokens;
    while (item >> token)
      tokens.push_back(token);
    if (tokens.size() < 2)
      throw Fatal("Invalid config file format: error_page requires at least 2 "
                  "arguments");
    std::string filename = tokens.back();
    for (size_t i = 0; i < tokens.size() - 1; ++i)
    {
      if (tokens[i].find_first_of("0123456789") == string::npos)
        throw Fatal(
            "Invalid config file format: error_page requires a number =>" +
            tokens[i]);
      if (tokens.back()[0] != '/')
        throw Fatal("Invalid config file format: error_page requires an "
                    "URI starting with '/' =>" +
                    tokens.back());
      int errcode = Utils::errStrToUint16(tokens[i]);
      if (!found_code(errcode))
      {
        throw Fatal("Invalid config file format: error code not found => " +
                    tokens[i]);
        continue;
      }
      if (config.error_pages.find(errcode) != config.error_pages.end())
      {
        std::cerr << WARNING
                  << "Duplicate error page for error code, statement "
                     "ignored => "
                  << errcode << " => " << filename << std::endl;
        continue;
      }
      config.error_pages[errcode] = filename;
    }
  }
  else
    throw Fatal("Invalid config file format: unknown token");
}

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Printing               ║
// ╚══════════════════════════════════════════════╝

void Configuration::printConfigurations() const
{
  std::cout << "=== Configuration ===" << std::endl;
  std::cout << "Global settings:" << std::endl;
  std::cout << "-->Cgi timeout: " << cgi_timeout_.first << std::endl;
  std::cout << "-->Keep alive timeout: " << keep_alive_timeout_.first
            << std::endl;
  std::cout << "server configs: " << std::endl;
  for (size_t i = 0; i < server_configs_.size(); ++i)
  {
    std::cout << server_configs_[i];
  }
  std::cout << std::endl;
}

std::ostream& operator<<(std::ostream& os, const IpSet& ips)
{
  for (IpSet::const_iterator it = ips.begin(); it != ips.end(); ++it)
  {
    IpAddress* addr = *it;
    if (addr->getType() == IPv4)
      os << *static_cast< Ipv4Address* >(addr);
    else
      os << *static_cast< Ipv6Address* >(addr);
    IpSet::const_iterator next = it;
    ++next;
    if (next != ips.end())
      os << ", ";
  }
  os << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const std::set< string >& server_names)
{
  for (ServerNames::const_iterator it = server_names.begin();
       it != server_names.end(); ++it)
  {
    os << *it;
    ServerNames::const_iterator next = it;
    ++next;
    if (next != server_names.end())
      os << ", ";
  }
  os << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const std::map< int, string >& error_pages)
{
  std::map< int, string >::const_iterator it = error_pages.begin();
  while (it != error_pages.end())
  {
    os << it->first << " => " << it->second;
    std::map< int, string >::const_iterator next = it;
    ++next;
    if (next != error_pages.end())
      os << ", ";
    else
      os << std::endl;
    ++it;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os,
                         const std::vector< string >& default_files)
{
  for (VDefaultFiles::const_iterator it = default_files.begin();
       it != default_files.end(); ++it)
  {
    os << *it;
    if (it + 1 != default_files.end())
      os << ", ";
    else
      os << std::endl;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const redirection& redirect)
{
  os << "------->Code: " << redirect.code << std::endl;
  os << "------->URI: " << redirect.uri << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const location& loc)
{
  os << std::endl;
  os << "---->Allowed http methods: " << (loc.GET ? "GET " : "")
     << (loc.POST ? "POST " : "") << (loc.DELETE ? "DELETE " : "") << std::endl;

  os << "---->Directory listing: " << (loc.DIR_LISTING.first ? "on" : "off")
     << std::endl;
  os << "---->Max body size: " << loc.max_body_size.first << std::endl;
  os << "---->CGI extensions: " << loc.cgi_extensions << std::endl;
  os << "---->Default files: " << loc.default_files << std::endl;
  os << "---->Redirection: " << std::endl;
  os << loc.redirect << std::endl;
  os << "---->Root: " << loc.root << std::endl;
  os << "---->Upload dir: " << loc.upload_dir << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const Server& config)
{
  os << ">Server config: " << std::endl;
  os << "-->Server names: " << config.server_names;
  os << "-->IPs: " << config.ips;
  os << "-->Error pages: " << config.error_pages;
  os << "-->All Locations: " << std::endl;
  for (MLocations::const_iterator it = config.locations.begin();
       it != config.locations.end(); ++it)
    os << "---> Locations " << it->first << it->second;
  return os;
}
