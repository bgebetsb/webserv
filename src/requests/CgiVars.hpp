#pragma once

#include <map>
#include <string>
#include "../requests/RequestMethods.hpp"

typedef std::map< std::string, std::string > mHeader;

struct CgiVars
{
  std::string input_file;
  RequestMethod request_method_enum_;
  long file_size;
  std::string request_method_str;
  std::string request_uri;
  std::string script_filename;
  std::string script_name;
  std::string path_info;
  std::string server_name;
  std::string server_port;
  std::string remote_addr;
  std::string document_root;
  std::string content_type;
  std::string query_string;
  mHeader headers;
};
