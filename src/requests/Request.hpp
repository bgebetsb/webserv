#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "../Configs/Configs.hpp"
#include "../Option.hpp"
#include "../responses/Response.hpp"
#include "RequestMethods.hpp"
#include "RequestStatus.hpp"

typedef std::map< std::string, std::string > mHeader;
typedef std::vector< Server > vServer;

const static std::string STANDARD_HEADERS[] = {"host", "content-length",
                                               "transfer-encoding"};

class Request
{
 public:
  Request(int fd, const std::vector< Server >& servers);
  Request(const Request& other);
  Request& operator=(const Request& other);
  ~Request();

  void addHeaderLine(const std::string& line);  // TODO: find out if we need to
                                                // add something for POST
  void processRequest(void);
  void sendResponse();
  void timeout();
  void setResponse(Response* response);

  RequestStatus getStatus() const;
  bool closingConnection() const;

  // ── ◼︎ utils ────────────────────────────────────────────────────────

 private:
  int fd_;
  RequestStatus status_;
  RequestMethod method_;
  std::string host_;
  std::string path_;
  mHeader headers_;
  bool chunked_;
  Option< long > content_length_;
  bool closing_;
  const vServer& servers_;
  size_t total_header_size_;
  Response* response_;
  // ── ◼︎ POST ───────────────────────

  // ── ◼︎ Start Line ───────────────────────
  void readStartLine(const std::string& line);
  void parseMethod(std::istringstream& stream);
  void parsePath(std::istringstream& stream);
  bool parseAbsoluteForm(const std::string& path);

  // Header
  Option< std::string > getHeader(const std::string& name) const;
  void parseHeaderLine(const std::string& line);
  void insertHeader(const std::string& key, const std::string& value);
  void validateHeaders(void);
  void processConnectionHeader(void);

  void processFilePath(const std::string& path, const location& location);
  void openFile(const std::string& path, off_t size);
  void openDirectory(const std::string& path, const location& location);
  const Server& getServer(const std::string& host) const;
  bool methodAllowed(const location& location) const;
  void validateTransferEncoding(const std::string& value);
  void validateContentLength(const std::string& value);
  // This one could be static
  const location& findMatchingLocationBlock(const MLocations& locations,
                                            const std::string& path) const;
  // ── ◼︎ POST UTILS───────────────────────
  bool isValidPostRequest(const location& loc) const;
  bool is_multipart;
  bool is_cgi;
};
