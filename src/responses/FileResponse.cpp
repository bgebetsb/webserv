#include "FileResponse.hpp"
#include "Connection.hpp"
#include "exceptions/ConError.hpp"
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

FileResponse::FileResponse(int client_fd, int file_fd, off_t size)
    : Response(client_fd, 200), file_fd_(file_fd), remaining_(size),
      rd_buf_(new char[CHUNK_SIZE]), eof_(false) {
  std::ostringstream response;

  response << createResponseHeaderLine() << "Content-Length: " << size
           << "\r\nConnection: ";
  if (close_connection_)
    response << "close\r\n\r\n";
  else
    response << "keep-alive\r\n\r\n";

  full_response_ = response.str();
}

FileResponse::~FileResponse() { delete[] rd_buf_; }

void FileResponse::sendResponse(void) {
  if (full_response_.size() < CHUNK_SIZE && remaining_ > 0 && !eof_) {
    size_t amount = std::min(static_cast<size_t>(CHUNK_SIZE),
                             static_cast<size_t>(remaining_));
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
