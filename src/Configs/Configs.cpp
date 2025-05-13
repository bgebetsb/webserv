#include "Configs.hpp"
#include <sys/types.h>

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../exceptions/Fatal.hpp"
#include "../ip/Ipv4Address.hpp"
#include "../utils/Utils.hpp"

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Con. / Destructors     ║
// ╚══════════════════════════════════════════════╝

Configuration::Configuration(const string& config_file)
{
  (void)config_file;
  parseConfigFile(config_file);
}

// Default constructor only enabled for testing purposes
Configuration::Configuration()
{
  serv_config config;
  config.cgi_timeout = std::make_pair(0, false);
  config.keep_alive_timeout = std::make_pair(0, false);
  server_configs_.push_back(config);
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

void Configuration::parseConfigFile(const string& config_file)
{
  (void)config_file;
  std::ifstream file(config_file.c_str());
  if (!file.is_open())
    throw Fatal("Could not open config file");

  string line;
  while (1)
  {
    std::getline(file, line, '{');
    if (file.eof())
      break;
    std::stringstream ss(line);
    std::string token;
    ss >> token;
    if (token != "server")
      throw Fatal("Ivalid config file format: expected 'server'");
    if (ss >> token)
      throw Fatal("Invalid config file format: expected '{'");
    std::getline(file, line, '}');
    if (file.eof())
      throw Fatal("Invalid config file format: expected '}'");
    process_server_block(line);
  }
}

void Configuration::process_server_block(const std::string& block)
{
  serv_config config;
  std::size_t cursor = 0;

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
    } else if (block[pos] == '{')
    {
      std::size_t end = block.find('}', pos);
      if (end == std::string::npos)
        throw Fatal("Invalid config file format: expected '}'");

      std::stringstream ss(block.substr(cursor, end - cursor + 1));
      process_location_block(ss, config);
      cursor = end + 1;  // weiter hinter der schließenden Klammer
    }
  }
}

void Configuration::process_location_block(std::stringstream& item,
                                           serv_config& config)
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
  config.locations[token] = new_location;
  std::getline(item, line, '}');
  if (line.empty())
    throw Fatal("Ivalid config file format: empty location block");
  std::stringstream ss2(line);
  while (std::getline(ss2, token, ';'))
  {
    if (token.empty())
      continue;
    std::stringstream ss3(token);
    process_location_item(ss3, config.locations[token]);
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
    tokens.push_back(identifier);

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
    if (tokens[0][0] != '/')
      throw Fatal("Invalid config file format: root requires an absolute path");
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
    if (tokens.size() == 1)
    {
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
      } else if (is_valid_redirection_code(code) && tokens.size() == 1)
        throw Fatal("Invalid config file format: return code => " + tokens[0] +
                    " should have a URI");
      else if (is_valid_redirection_code(code) && tokens.size() == 2)
      {
        loc.redirect.code = code;
        if (tokens[1][0] != '/')
          throw Fatal("Invalid config file format: return URI => " + tokens[1] +
                      " should start with '/'");
        loc.redirect.uri = tokens[1];
        loc.redirect.has_been_set = true;
      }
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
  else
    throw Fatal("Invalid config file format: unknown token in location block");
}

static void insert_ip(IpSet& ips, const string& token)
{
  if (token.find_first_of(".:[]") == string::npos)
  {
    u_int16_t port = Utils::ipStrToUint16(token);
    Ipv4Address* addr = new Ipv4Address(0, port);
    if (!ips.insert(addr).second)
    {
      delete addr;
      throw Fatal("Invalid config file format: duplicate IP address");
    }
  }
  if (token[0] == '[')
    ;
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
      if (is_valid_server_name(tokens[i]))
      {
        if (config.server_names.insert(tokens[i]).second == false)
        {
          std::cerr << WARNING << "Duplicate server name, statement ignored => "
                    << tokens[i] << std::endl;
          continue;
        }
      } else
        throw Fatal("Invalid config file format: invalid server name => " +
                    tokens[i]);
    }
  }

  // ── ◼︎ listen ─────────────────────────────────────────────────────────────
  else if (token == "listen")
  {
    if (config.ips.size() > 0)
      throw Fatal("Invalid config file format: listen already defined");
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
  } else
    throw Fatal("Invalid config file format: unknown token");
}
