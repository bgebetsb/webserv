#pragma once

// ── ◼︎ includes ─────────────────────────────
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../ip/IpAddress.hpp"

// ── ◼︎ typedefs names ───────────────────────
typedef int port;
typedef int errcode;
typedef std::string filename;

// ── ◼︎ typedefs utils ───────────────────────
typedef std::string string;
typedef std::set< IpAddress* > IpSet;
typedef std::set< string > ServerNames;
typedef std::map< errcode, filename > MErrors;
typedef std::map< string, string > MRedirects;
typedef std::vector< filename > VDefaultFiles;
typedef std::map< string, string > VCgiPaths;
struct location;
typedef std::vector< location > VLocations;

/*
 * ── ◼︎ typedefs http methods ────────────────
 <bool, has_been_set> __: true if the method has been set;
*/
typedef std::pair< bool, bool > bool_pair;

/*
 * ── ◼︎ typedefs size_t ________________
 * <size_t, has_been_set> __: true if the size has been set;
 */
typedef std::pair< size_t, bool > size_pair;
// ── ◼︎ structs ──────────────────────────────

/*
 * ── ◼︎ struct location ──────────────────────
 *
 * This struct represents a location block in the configuration file.
 * GET, POST, DELETE <bool> ___: Allowed HTTP methods;
 * DIR_LISTING <bool> _________: Directory listing enabled or not;
 * max_body_size <size_t> ______: Maximum body size for requests;
 * cgi_paths <VCgiPaths> _______: Vec<string> of cgi paths;
 * default_files <VDefaultFiles>__: Vec<string> of default files to serve;
 * redirects <MRedirects> _______: Map<string, string> of redirects;
 * root <string> ________________: Root directory for the location;
 */
struct location
{
  bool_pair GET;                // http methods
  bool_pair POST;               // http methods
  bool_pair DELETE;             // http methods
  bool_pair DIR_LISTING;        // dir_listing active or not
  size_pair max_body_size;      // in bytes
  VCgiPaths cgi_paths;          // cgi_paths
  VDefaultFiles default_files;  // default_files
  MRedirects redirects;         // redirections
  string root;                  // root
};

/*
 * ── ◼︎ struct serv_config ────────────────────
 *
 * This struct represents a server block in the configuration file.
 * cgi_timeout <size_t> ________: Timeout for CGI scripts;
 * keep_alive_timeout <size_t>__: Timeout for keep-alive connections;
 * server_names <ServerNames;> _____: Vec<string> of server names;
 * ips <IpVec> ________________: Vec<IpAddress*> of IP addresses;
 * error_pages <MErrors> ______: Map<errcode, filename> of error pages;
 * locations <VLocations> ______: Vec<location> of locations;
 */
struct serv_config
{
  size_pair cgi_timeout;         // cgi_timeout //TODO: default config
  size_pair keep_alive_timeout;  // keep_alive_timeout //TODO: default config
  ServerNames server_names;      // server_name
  IpSet ips;                     // ipv4 or ipv6
  MErrors error_pages;           // error_pages
  VLocations locations;          // locations
};

typedef std::vector< serv_config > ServerVec;

// TODO: Defend for config file dev/random dev/urandom

class Configuration
{
 public:
  Configuration(const std::string& config_file);
  ~Configuration();

  void parseConfigFile(const std::string& config_file);
  void process_server_block(const std::string& line);
  void process_server_item(std::stringstream& item, serv_config& config);
  void addServer(const std::string& server_name, int port);

 private:
  ServerVec server_configs_;

  Configuration();
};
