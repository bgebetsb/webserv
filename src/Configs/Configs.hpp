#pragma once

// ── ◼︎ includes ─────────────────────────────
#include <sys/types.h>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../ip/IpAddress.hpp"

#define CGI_TIMEOUT_MAX 300
#define KEEP_ALIVE_TIMEOUT_MAX 60

// ── ◼︎ errorcodes implemented ───────────────────────// TODO: check if complete
static const u_int16_t error_codes[] = {403, 404, 405, 500};

// ── ◼︎ invalid chars for servername ───────────────────────
static const char invalid_server_name_chars[] = {
    '=', '{', '}', '[', ']', '\\', '|', ';', ':', '\'', '"', '`', '!', '@', '#',
    '$', '%', '^', '&', '*', '(',  ')', '+', ',', '<',  '>', '/', '?', '~'};

// ── ◼︎ supported cgi extensions ───────────────────────
static const std::string cgis[] = {".py", ".php"};

// ── ◼︎ valid http codes ───────────────────────
const int VALID_HTTP_CODES[] = {
    // 1xx Informational
    100, 101, 102, 103,
    // 2xx Success
    200, 201, 202, 203, 204, 205, 206, 207, 208, 226,
    // 3xx Redirection
    300, 301, 302, 303, 304, 305, 306, 307, 308,
    // 4xx Client Errors
    400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414,
    415, 416, 417, 418, 421, 422, 423, 424, 425, 426, 428, 429, 431, 451,
    // 5xx Server Errors
    500, 501, 502, 503, 504, 505, 506, 507, 508, 510, 511};
const std::size_t VALID_HTTP_CODES_COUNT =
    sizeof(VALID_HTTP_CODES) / sizeof(VALID_HTTP_CODES[0]);

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

/*
 * If there is no access_log configured, it will be in state `LogFile` but the
 * file will actually be `/dev/null`
 */
enum LogMode
{
  LOGFILE,
  STDOUT,
  STDERR
};

struct LogSettings
{
  LogSettings() : configured(false), mode(LOGFILE) {}

  bool configured;
  LogMode mode;
  std::string logfile;
};

// ── ◼︎ typedefs utils ───────────────────────
typedef std::string string;
typedef std::set< IpAddress* > IpSet;
typedef std::set< string > ServerNames;
typedef std::map< errcode, filename > MErrors;
typedef std::map< string, string > MRedirects;
typedef std::vector< filename > VDefaultFiles;
typedef std::set< string > SCgiExtensions;
struct Location;
typedef std::map< string, Location, std::greater< string > > MLocations;

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

struct Redirection
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
struct Location
{
  Location()
      : http_methods_set(false),
        GET(false),
        POST(false),
        DELETE(false),
        DIR_LISTING(false, false),
        max_body_size(0, false),
        cgi_extensions(),
        default_files(),
        redirect(),
        root(),
        upload_dir()
  {}
  bool http_methods_set;
  bool GET;                       // http methods
  bool POST;                      // http methods
  bool DELETE;                    // http methods
  bool_pair DIR_LISTING;          // dir_listing active or not
  size_pair max_body_size;        // in bytes
  SCgiExtensions cgi_extensions;  // cgi_extensions
  VDefaultFiles default_files;    // default_files
  Redirection redirect;           // redirections
  string root;                    // root
  string upload_dir;              // upload_dir
  string location_name;           // location name
};

/// @brief `Server configuration`
///
/// `_______cgi_timeout` timeout for cgi
/// `keep_alive_timeout` timeout for keep alive
/// `______server_names` server names
/// `_______________ips` ip addresses
/// `_______error_pages` error pages
/// `_________locations` locations
struct Server
{
  ServerNames server_names;  // server_name
  IpSet ips;                 // ipv4 or ipv6
  MErrors error_pages;       // error_pages
  MLocations locations;      // locations
};

typedef std::vector< Server > ServerVec;

class Configuration
{
 private:
  ServerVec server_configs_;
  size_pair cgi_timeout_;         // cgi_timeout //TODO: default config
  size_pair keep_alive_timeout_;  // keep_alive_timeout //TODO: default config
  LogSettings log_;
  const string config_file_;

 public:
  // ── ◼︎ Constructors / Destructor ───────────
  Configuration();
  Configuration(const std::string& config_file);
  ~Configuration();

  // ── ◼︎ Config file parsing ─────────────────
  void checkFileType(const std::string& filename) const;
  std::string removeComments(const std::string& content) const;
  void parseConfigFile(const std::string& config_file);
  void process_server_block(const std::string& line);
  void process_server_item(std::stringstream& item, Server& config);
  void process_location_block(std::stringstream& item, Server& loc);
  void process_location_item(std::stringstream& item, Location& loc);
  void addServer(const std::string& server_name, int port);
  void printConfigurations() const;

  // ── ◼︎ Config file getters ─────────────────
  const ServerVec& getServerConfigs() const
  {
    return server_configs_;
  }

  const LogSettings& getLogsettings() const
  {
    return log_;
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
  static bool is_valid_cgi(const string& cgi_path)
  {
    for (size_t i = 0; i < sizeof(cgis) / sizeof(string); ++i)
    {
      if (cgi_path == cgis[i])
        return true;
    }
    return false;
  }
  static bool is_valid_redirection_code(int code)
  {
    for (size_t i = 0; i < sizeof(VALID_HTTP_CODES) / sizeof(int); ++i)
    {
      if (VALID_HTTP_CODES[i] == code)
        return true;
    }
    return false;
  }
};

// ── ◼︎ operator overloads ───────────────────────
std::ostream& operator<<(std::ostream& os, const Server& config);
std::ostream& operator<<(std::ostream& os, const Location& loc);
std::ostream& operator<<(std::ostream& os, const Redirection& redirect);
std::ostream& operator<<(std::ostream& os,
                         const std::vector< string >& default_files);
std::ostream& operator<<(std::ostream& os,
                         const std::map< int, string >& error_pages);
std::ostream& operator<<(std::ostream& os,
                         const std::set< string >& server_names);
std::ostream& operator<<(std::ostream& os, const IpSet& ips);

#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define WARNING YELLOW "[Warning]: " RESET
