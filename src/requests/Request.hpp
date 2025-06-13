#pragma once

#include <sys/types.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "../Configs/Configs.hpp"
#include "../Option.hpp"
#include "../responses/Response.hpp"
#include "../utils/FdWrap.hpp"
#include "CgiVars.hpp"
#include "RequestMethods.hpp"
#include "RequestStatus.hpp"

typedef std::map< std::string, std::string > mHeader;
typedef std::vector< Server > vServer;

const static std::string STANDARD_HEADERS[] = {"host", "content-length",
                                               "transfer-encoding"};

enum UploadMode
{
  NORM,
  PROGRESS,
  TRAILER,
  END,
  ERROR_LENGTH,
  ERROR_CHUNKSIZE,
  CLEAN
};

enum CgiExtension
{
  PHP,
  PYTHON
};

class Request
{
  // ── ◼︎ member variables
  // ───────────────────────
 private:
  int fd_;
  const std::string& client_ip_;
  const Server* server_;
  RequestStatus status_;
  RequestMethod method_;
  std::string uri_;
  std::string host_;
  std::string path_;
  std::string path_info_;
  std::string port_;
  std::string startline_;
  std::string query_string_;
  mHeader headers_;
  bool chunked_;
  Option< long > content_length_;
  bool closing_;
  const vServer& servers_;
  size_t total_header_size_;
  Response* response_;

  // ── ◼︎ constructors, destructors, assignment
  // ───────────────────────
 public:
  Request(int fd,
          const std::vector< Server >& servers,
          const std::string& client_ip);
  Request(const Request& other);
  Request& operator=(const Request& other);
  ~Request();

  // ── ◼︎ Request
  // ───────────────────────
  void addHeaderLine(const std::string& line);
  void processRequest(void);

 private:
  bool isFileUpload(const Location& loc);
  void setupFileUpload();
  void setupCgi();

  std::string generateRandomFilename();
  // ── ◼︎ POST
  // ───────────────────────
 public:
  void uploadBody(const std::string& body, UploadMode mode = NORM);

 private:
  long max_body_size_;
  bool is_cgi_;
  std::string filename_;
  std::string absolute_path_;
  Utils::FdWrap upload_file_;
  std::string cgi_script_filename_;
  std::string cgi_script_name_;
  std::string document_root_;
  bool file_existed_;
  static std::set< std::string > current_upload_files_;
  CgiExtension cgi_extension_;
  long total_written_bytes_;
  std::string upload_dir_;
  // ── ◼︎ Response
  // ───────────────────────
 public:
  void sendResponse();
  void timeout();
  void setResponse(Response* response);

  // ── ◼︎ getters
  // ───────────────────────
  RequestStatus getStatus() const;
  bool closingConnection() const;
  const Server& getServer() const;
  const std::string& getStartLine() const;
  const std::string& getHost() const;
  u_int16_t getResponseCode() const;
  long getMaxBodySize() const;
  bool isChunked() const;
  long getContentLength() const;

  // ── ◼︎ utils
  // ────────────────────────────────────────────────────────
  static const Location& findMatchingLocationBlock(const MLocations& locations,
                                                   const std::string& path);

  // ── ◼︎ Start Line
  // ───────────────────────
  void readStartLine(const std::string& line);
  void parseMethod(std::istringstream& stream);
  void parsePath(std::istringstream& stream);
  void parseAbsoluteForm(const std::string& path);

  // ── ◼︎ Header
  // ───────────────────────
  Option< std::string > getHeader(const std::string& name) const;
  void processHeaderLine(const std::string& line);
  void insertHeader(const std::string& key, const std::string& value);
  void validateHeaders(void);
  void processConnectionHeader(void);

  // ── ◼︎ xxx
  // ───────────────────────
  // //TODO: rename this
  void checkForCgi(const Location& loc);
  void processFilePath(const std::string& path, const Location& location);
  int openFile(const std::string& path) const;
  void openDirectory(const std::string& path, const Location& location);
  void createDirectoryListing(const std::string& path);
  const Server& getServer(const std::string& host) const;
  bool methodAllowed(const Location& location) const;
  void validateTransferEncoding(const std::string& value);
  void validateContentLength(const std::string& value);
  CgiVars createCgiVars(void) const;
  void splitPathInfo(const std::string& server_root);
};
