/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Post.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbonengl <mbonengl@student.42vienna.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/20 19:25:39 by mbonengl          #+#    #+#             */
/*   Updated: 2025/05/31 19:21:15 by mbonengl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <string>
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
#include "Option.hpp"
#include "Request.hpp"
#include "RequestStatus.hpp"
#include "exceptions/RequestError.hpp"

std::string Request::generateRandomFilename()
{
  std::string filename;
  const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t length = 16;  // Length of the random filename
  for (size_t i = 0; i < length; ++i)
  {
    size_t index = std::rand() % (sizeof(charset) - 1);
    filename += charset[index];
  }
  return filename;
}

void Request::uploadBody(const std::string& body, UploadMode mode)
{
  if (!upload_file_.is_open())
    throw RequestError(500, "Upload file not open");
  if (mode == ERROR_LENGTH)
  {
    upload_file_.close();
    std::remove(absolute_path_.c_str());
    throw RequestError(413, "Request body too large");
  }
  if (mode == ERROR_CHUNKSIZE)
  {
    upload_file_.close();
    std::remove(absolute_path_.c_str());
    throw RequestError(400, "Invalid chunk size");
  }
  upload_file_.write(body.c_str(), body.size());
  if (upload_file_.bad())
  {
    upload_file_.close();
    std::remove(absolute_path_.c_str());
    throw RequestError(500, "Failed to write to upload file");
  }
  if (mode == END)
  {
    response_ = new StaticResponse(fd_, 200, closing_);
    upload_file_.close();
    status_ = SENDING_RESPONSE;
  }
}
