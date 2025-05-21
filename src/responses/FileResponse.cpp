#include "FileResponse.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "Connection.hpp"
#include "exceptions/ConError.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/PathValidation/FileTypes.hpp"
#include "requests/PathValidation/PathInfos.hpp"
#include "requests/PathValidation/PathValidation.hpp"

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
           << "\r\nConnection: ";
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
