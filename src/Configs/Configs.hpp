#pragma once

// ── ◼︎ includes ─────────────────────────────
#include <sys/types.h>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../Server.hpp"
#include "../ip/IpAddress.hpp"

// ── ◼︎ errorcodes     ───────────────────────
static const u_int16_t error_codes[] = {400, 403, 404, 405, 408, 500};
static const char invalid_server_name_chars[] = {
    '=', '{', '}', '[', ']', '\\', '|', ';', ':', '\'', '"', '`', '!', '@', '#',
    '$', '%', '^', '&', '*', '(',  ')', '+', ',', '<',  '>', '/', '?', '~'};
static const std::string cgis[] = {".py", ".php"};
static const u_int16_t redirection_codes[] = {301, 302, 303, 307,
                                              308};  // redirection codes

// ── ◼︎ typedefs names ───────────────────────
typedef int port;
typedef int errcode;
typedef std::string filename;

enum eCGI
{
  CGI_PHP,
  CGI_PYTHON,
  CGI_NONE
};

// ── ◼︎ typedefs utils ───────────────────────
typedef std::string string;
typedef std::set< IpAddress* > IpSet;
typedef std::set< string > ServerNames;
typedef std::map< errcode, filename > MErrors;
typedef std::map< string, string > MRedirects;
typedef std::vector< filename > VDefaultFiles;
typedef std::set< string > SCgiExtensions;
struct location;
typedef std::map< string, location > MLocations;

/// @brief `Pair of bools to indicate if a value has been set`
///
/// `<bool>` -> `value` , `<bool>` -> `has_been_set`
/// set;
typedef std::pair< bool, bool > bool_pair;

/// @brief `Pair of size_t and bool to indicate if a value has been set`
///
/// `<size_t>` `value` , `<bool>` `has_been_set`
typedef std::pair< size_t, bool > size_pair;

// ╔══════════════════════════════════════════════╗
// ║              SECTION: Your Section           ║
// ╚══════════════════════════════════════════════╝

struct rediection
{
  bool has_been_set;
  u_int16_t code;
  string uri;
};

/// @brief Location configuration
///
/// Booleans for HTTP methods:
/// - `GET`
/// - `POST`
/// - `DELETE`
/// `___DIR_LISTING` if directory listing is allowed
/// `_max_body_size` maximum body size
/// `cgi_extensions` cgi extensions
/// `_default_files` default files
/// `_____redirects` redirections
/// `__________root` root directory
/// `____upload_dir` upload directory
struct location
{
  bool http_methods_set;
  bool GET;                       // http methods
  bool POST;                      // http methods
  bool DELETE;                    // http methods
  bool_pair DIR_LISTING;          // dir_listing active or not
  size_pair max_body_size;        // in bytes
  SCgiExtensions cgi_extensions;  // cgi_extensions
  VDefaultFiles default_files;    // default_files
  rediection redirect;            // redirections
  string root;                    // root
  string upload_dir;              // upload_dir
};

/// @brief `Server configuration`
///
/// `_______cgi_timeout` timeout for cgi
/// `keep_alive_timeout` timeout for keep alive
/// `______server_names` server names
/// `_______________ips` ip addresses
/// `_______error_pages` error pages
/// `_________locations` locations
struct serv_config
{
  size_pair cgi_timeout;         // cgi_timeout //TODO: default config
  size_pair keep_alive_timeout;  // keep_alive_timeout //TODO: default config
  ServerNames server_names;      // server_name
  IpSet ips;                     // ipv4 or ipv6
  MErrors error_pages;           // error_pages
  MLocations locations;          // locations
};

typedef std::vector< serv_config > ServerVec;

// TODO: Defend for config file dev/random dev/urandom

class Configuration
{
 private:
  ServerVec server_configs_;

 public:
  // ── ◼︎ Constructors / Destructor ───────────
  Configuration();
  Configuration(const std::string& config_file);
  ~Configuration();

  // ── ◼︎ Config file parsing ─────────────────
  void parseConfigFile(const std::string& config_file);
  void process_server_block(const std::string& line);
  void process_server_item(std::stringstream& item, serv_config& config);
  void process_location_block(std::stringstream& item, serv_config& loc);
  void process_location_item(std::stringstream& item, location& loc);
  void addServer(const std::string& server_name, int port);

  // ── ◼︎ Config file getters ─────────────────
  ServerVec getServerConfigs() const
  {
    return server_configs_;
  }

  // ── ◼︎ Utilities  ───────────────────────
  static bool found_code(int code)
  {
    for (size_t i = 0; i < sizeof(error_codes) / sizeof(u_int16_t); ++i)
    {
      if (error_codes[i] == code)
        return true;
    }
    return false;
  }
  static bool is_valid_server_name(const string& server_name)
  {
    for (size_t i = 0; i < sizeof(invalid_server_name_chars) /
                               sizeof(invalid_server_name_chars[0]);
         ++i)
    {
      if (server_name.find(invalid_server_name_chars[i]) != string::npos)
        return false;
    }
    return true;
  }
  static bool is_valid_cgi(const string& cgi_path)
  {
    for (size_t i = 0; i < sizeof(cgis) / sizeof(string); ++i)
    {
      if (cgi_path.find(cgis[i]) != string::npos)
        return true;
    }
    return false;
  }
  static bool is_valid_redirection_code(int code)
  {
    for (size_t i = 0; i < sizeof(redirection_codes) / sizeof(u_int16_t); ++i)
    {
      if (redirection_codes[i] == code)
        return true;
    }
    return false;
  }
};

#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define WARNING YELLOW "[Warning]: " RESET
