#include "Configs.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include "../exceptions/Fatal.hpp"
#include "../ip/Ipv4Address.hpp"
#include "../utils/Utils.hpp"
Configuration::Configuration(const string& config_file)
{
  (void)config_file;
  parseConfigFile(config_file);
}

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

void Configuration::process_server_block(const string& line)
{
  (void)line;
  serv_config config;

  while (1)
  {
    string::size_type pos = line.find_first_of(";{");
    if (pos == string::npos)
      break;
    if (line[pos] == ';')
    {
      std::stringstream ss(line.substr(0, pos));
      process_server_item(ss, config);
    } else if (line[pos] == '{')
    {}
  }
}

static void insert_ip(IpSet& ips, const string& token)
{
  if (token.find_first_of(".:[]") == string::npos)
  {
    u_int16_t port = Utils::strtouint16(token);
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
  if (token == "server_name")
  {
    if (config.server_names.size() > 0)
      throw Fatal("Invalid config file format: server_name already defined");
    while (item >> token)
      config.server_names.insert(token);
  }
  if (token == "listen")  // no dup
  {
    if (config.ips.size() > 0)
      throw Fatal("Invalid config file format: listen already defined");
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
  {
  } else
  {
    std::cout << "Not found" << std::endl;
  }
}
