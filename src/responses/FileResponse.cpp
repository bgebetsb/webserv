#include "FileResponse.hpp"
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include "epoll/Connection.hpp"
#include "exceptions/ConError.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/PathValidation/FileTypes.hpp"
#include "requests/PathValidation/PathInfos.hpp"
#include "requests/PathValidation/PathValidation.hpp"

/*
 * https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/MIME_types/Common_types
 */
static MMimeTypes createMimeTypesMap()
{
  MMimeTypes map;

  map.insert(std::make_pair(".aac", "audio/aac"));
  map.insert(std::make_pair(".abw", "application/x-abiword"));
  map.insert(std::make_pair(".apng", "image/apng"));
  map.insert(std::make_pair(".arc", "application/x-freearc"));
  map.insert(std::make_pair(".avif", "image/avif"));
  map.insert(std::make_pair(".avi", "video/x-msvideo"));
  map.insert(std::make_pair(".azw", "application/vnd.amazon.ebook"));
  map.insert(std::make_pair(".bin", "application/octet-stream"));
  map.insert(std::make_pair(".bmp", "image/bmp"));
  map.insert(std::make_pair(".bz", "application/x-bzip"));
  map.insert(std::make_pair(".bz2", "application/x-bzip2"));
  map.insert(std::make_pair(".cda", "application/x-cdf"));
  map.insert(std::make_pair(".csh", "application/x-csh"));
  map.insert(std::make_pair(".css", "text/css"));
  map.insert(std::make_pair(".csv", "text/csv"));
  map.insert(std::make_pair(".doc", "application/msword"));
  map.insert(std::make_pair(
      ".docx", "application/"
               "vnd.openxmlformats-officedocument.wordprocessingml.document"));
  map.insert(std::make_pair(".eot", "application/vnd.ms-fontobject"));
  map.insert(std::make_pair(".epub", "application/epub+zip"));
  map.insert(std::make_pair(".gz", "application/gzip"));
  map.insert(std::make_pair(".gif", "image/gif"));
  map.insert(std::make_pair(".htm", "text/html"));
  map.insert(std::make_pair(".html", "text/html"));
  map.insert(std::make_pair(".ico", "image/vnd.microsoft.icon"));
  map.insert(std::make_pair(".ics", "text/calendar"));
  map.insert(std::make_pair(".jar", "application/java-archive"));
  map.insert(std::make_pair(".jpeg", "image/jpeg"));
  map.insert(std::make_pair(".jpg", "image/jpeg"));
  map.insert(std::make_pair(
      ".js", "text/javascript (Specifications: HTML and RFC 9239)"));
  map.insert(std::make_pair(".json", "application/json"));
  map.insert(std::make_pair(".jsonld", "application/ld+json"));
  map.insert(std::make_pair(".md", "text/markdown"));
  map.insert(std::make_pair(".mid", "audio/midi"));
  map.insert(std::make_pair(".midi", "audio/midi"));
  map.insert(std::make_pair(".mjs", "text/javascript"));
  map.insert(std::make_pair(".mp3", "audio/mpeg"));
  map.insert(std::make_pair(".mp4", "video/mp4"));
  map.insert(std::make_pair(".mpeg", "video/mpeg"));
  map.insert(std::make_pair(".mpkg", "application/vnd.apple.installer+xml"));
  map.insert(std::make_pair(".odp",
                            "application/vnd.oasis.opendocument.presentation"));
  map.insert(
      std::make_pair(".ods", "application/vnd.oasis.opendocument.spreadsheet"));
  map.insert(std::make_pair(".odt", "application/vnd.oasis.opendocument.text"));
  map.insert(std::make_pair(".oga", "audio/ogg"));
  map.insert(std::make_pair(".ogv", "video/ogg"));
  map.insert(std::make_pair(".ogx", "application/ogg"));
  map.insert(std::make_pair(".opus", "audio/ogg"));
  map.insert(std::make_pair(".otf", "font/otf"));
  map.insert(std::make_pair(".png", "image/png"));
  map.insert(std::make_pair(".pdf", "application/pdf"));
  map.insert(std::make_pair(".php", "application/x-httpd-php"));
  map.insert(std::make_pair(".ppt", "application/vnd.ms-powerpoint"));
  map.insert(std::make_pair(
      ".pptx",
      "application/"
      "vnd.openxmlformats-officedocument.presentationml.presentation"));
  map.insert(std::make_pair(".rar", "application/vnd.rar"));
  map.insert(std::make_pair(".rtf", "application/rtf"));
  map.insert(std::make_pair(".sh", "application/x-sh"));
  map.insert(std::make_pair(".svg", "image/svg+xml"));
  map.insert(std::make_pair(".tar", "application/x-tar"));
  map.insert(std::make_pair(".tif", "image/tiff"));
  map.insert(std::make_pair(".tiff", "image/tiff"));
  map.insert(std::make_pair(".ts", "video/mp2t"));
  map.insert(std::make_pair(".ttf", "font/ttf"));
  map.insert(std::make_pair(".txt", "text/plain"));
  map.insert(std::make_pair(".vsd", "application/vnd.visio"));
  map.insert(std::make_pair(".wav", "audio/wav"));
  map.insert(std::make_pair(".weba", "audio/webm"));
  map.insert(std::make_pair(".webm", "video/webm"));
  map.insert(std::make_pair(".webp", "image/webp"));
  map.insert(std::make_pair(".woff", "font/woff"));
  map.insert(std::make_pair(".woff2", "font/woff2"));
  map.insert(std::make_pair(".xhtml", "application/xhtml+xml"));
  map.insert(std::make_pair(".xls", "application/vnd.ms-excel"));
  map.insert(std::make_pair(
      ".xlsx",
      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
  map.insert(std::make_pair(".xml", "application/xml"));
  map.insert(std::make_pair(".xul", "application/vnd.mozilla.xul+xml"));
  map.insert(std::make_pair(".zip", "application/zip"));
  map.insert(std::make_pair(".3gp", "video/3gpp"));
  map.insert(std::make_pair(".3g2", "video/3gpp2"));
  map.insert(std::make_pair(".7z", "application/x-7z-compressed"));

  return map;
}

const MMimeTypes FileResponse::mime_types_ = createMimeTypesMap();

FileResponse::FileResponse(int client_fd,
                           const std::string& filename,
                           int response_code,
                           bool close)
    : Response(client_fd, response_code, close),
      headers_created_(false),
      rd_buf_(new char[CHUNK_SIZE]),
      eof_(false)
{
  try
  {
    openFile(filename);
    content_type_ = detectContentType(filename);
  }
  catch (std::exception& e)
  {
    delete[] rd_buf_;
    throw;
  }
}

void FileResponse::createHeaders()
{
  std::ostringstream response;

  response << createResponseHeaderLine() << "Content-Length: " << remaining_
           << "\r\nContent-Type: " << content_type_ << "\r\nConnection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";

  full_response_ = response.str();
  headers_created_ = true;
}

void FileResponse::openFile(const std::string& filename)
{
  PathInfos infos = getFileType(filename);
  if (infos.exists && infos.types == REGULAR_FILE && !infos.readable)
    throw RequestError(403, "File is not readable");
  else if (!infos.exists || infos.types != REGULAR_FILE)
    throw RequestError(404, "File doesn't exist or isn't a regular file");

  file_fd_ = open(filename.c_str(), O_RDONLY | O_NOFOLLOW);
  if (file_fd_ == -1)
  {
    if (errno == ELOOP)
      throw RequestError(403, "Requested file is a symlink");
    else if (errno == ENFILE || errno == EMFILE)
      throw RequestError(503, "Server ran out of fds");
    throw RequestError(500, "Open failed for unknown reason");
  }
  remaining_ = infos.size;
}

std::string FileResponse::detectContentType(const std::string& filename) const
{
  std::string::size_type pos = filename.find_last_of('.');

  MMimeTypes::const_iterator it;
  if (pos == std::string::npos)
    return ("application/octet-stream");
  it = mime_types_.find(filename.substr(pos));
  if (it == mime_types_.end())
    return ("application/octet-stream");
  return it->second;
}

FileResponse::~FileResponse()
{
  delete[] rd_buf_;
  close(file_fd_);
}

void FileResponse::sendResponse(void)
{
  if (!headers_created_)
    createHeaders();

  if (full_response_.size() < CHUNK_SIZE && remaining_ > 0 && !eof_)
  {
    size_t amount = std::min(static_cast< size_t >(CHUNK_SIZE),
                             static_cast< size_t >(remaining_));
    ssize_t ret = read(file_fd_, rd_buf_, amount);
    if (ret == -1)
      throw ConErr("Read failed");

    eof_ = (ret == 0 || ret == remaining_);
    if (ret == 0)
      close_connection_ = true;

    remaining_ -= ret;
    full_response_.append(rd_buf_, ret);
  }

  Response::sendResponse();
}
