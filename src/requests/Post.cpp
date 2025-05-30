/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Post.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbonengl <mbonengl@student.42vienna.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/20 19:25:39 by mbonengl          #+#    #+#             */
/*   Updated: 2025/05/30 20:24:56 by mbonengl         ###   ########.fr       */
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
#include "../utils/Utils.hpp"
#include "Configs/Configs.hpp"
#include "Option.hpp"
#include "PathValidation/FileTypes.hpp"
#include "PathValidation/PathInfos.hpp"
#include "PathValidation/PathValidation.hpp"
#include "Request.hpp"
#include "RequestStatus.hpp"
#include "exceptions/RequestError.hpp"
#include "requests/RequestMethods.hpp"
#include "responses/FileResponse.hpp"
#include "responses/Response.hpp"

std::string generateRandomFilename()
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

void Request::uploadBody(const std::string& body)
{
  if (!upload_file_.is_open())
    throw RequestError(500, "Upload file not open");
  if (content_length_.is_some())
    uploadBodyWithContentLength(body);
  else if (chunked_)
    ;  // uploadBodyChunked(body);
}

void Request::uploadBodyWithContentLength(const std::string& body)
{
  if (content_length_.unwrap() < 0)
    throw RequestError(400, "Negative Content-Length header");
  if (content_length_.unwrap() > max_body_size_)
    throw RequestError(413, "Content-Length exceeds maximum body size");
  if (total_written_bytes_ + static_cast< long >(body.size()) >
      content_length_.unwrap())
    throw RequestError(413, "Content-Length exceeded during upload");
  upload_file_.write(body.c_str(), body.size());
  if (!upload_file_)
  {
    throw RequestError(500, "Failed to write to upload file");
  }
  total_written_bytes_ += body.size();
  if (total_written_bytes_ == content_length_.unwrap() && !is_cgi_)
  {
    upload_file_.close();
    status_ = SENDING_RESPONSE;
    response_ =
        new StaticResponse(fd_, 200, closing_, "File uploaded successfully");
  }
  else if (total_written_bytes_ > content_length_.unwrap())
  {
    throw RequestError(413, "Content-Length exceeded during upload");
  }
  else if (is_cgi_)
  {
    ;
  }
}
