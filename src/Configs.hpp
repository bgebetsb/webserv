#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "ip/IpAddress.hpp"
// ── ◼︎ typedefs names ───────────────────────
typedef int port;
typedef int errcode;
typedef std::string filename;

// ── ◼︎ typedefs utils ───────────────────────
typedef std::string string;
typedef std::vector< IpAddress* > IpVec;
typedef std::vector< string > Strings;
typedef std::map< errcode, filename > MErrors;
typedef std::map< string, string > MRedirects;
typedef std::vector< filename > VDefaultFiles;
typedef std::map< string, string > VCgiPaths;
struct location;
typedef std::vector< location > VLocations;
// ── ◼︎ structs ──────────────────────────────
struct location
{
  bool GET;                     // http methods
  bool POST;                    // http methods
  bool DELETE;                  // http methods
  bool DIR_LISTING;             // dir_listing active or not
  size_t max_body_size;         // in bytes
  VCgiPaths cgi_paths;          // cgi_paths
  VDefaultFiles default_files;  // default_files
  MRedirects redirects;         // redirections
  string root;                  // root
};

struct serv_config
{
  size_t cgi_timeout;         // cgi_timeout //TODO: default config
  size_t keep_alive_timeout;  // keep_alive_timeout //TODO: default config
  Strings server_names;       // server_name
  IpVec ips;                  // ipv4 or ipv6
  MErrors error_pages;        // error_pages
  VLocations locations;       // locations
};

typedef std::vector< serv_config > ServerVec;

// TODO: Defend for config file dev/random dev/urandom

class Configuration
{
 public:
  Configuration(const std::string& config_file);
  ~Configuration();

  void parseConfigFile(const std::string& config_file);
  void addServer(const std::string& server_name, int port);

 private:
  ServerVec server_configs_;

  Configuration();
};
