/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Post.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbonengl <mbonengl@student.42vienna.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/20 19:25:39 by mbonengl          #+#    #+#             */
/*   Updated: 2025/06/03 20:59:38 by mbonengl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fcntl.h>
#include <sys/socket.h>
#include <cstddef>
#include <cstdlib>
#include <string>
#include "../Configs/Configs.hpp"
#include "../responses/CgiResponse.hpp"
#include "../responses/RedirectResponse.hpp"
#include "../responses/StaticResponse.hpp"
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
  if (mode == CLEAN)
  {
    if (upload_file_.is_open())
    {
      upload_file_.close();
      std::remove(absolute_path_.c_str());
    }
    status_ = READING_START_LINE;
    return;
  }
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
    if (is_cgi_)
    {
      if (current_upload_files_.erase(absolute_path_) == 0)
        throw RequestError(500, "File not found in current uploads");
      std::string cgi_bin_path;
      cgi_extension_ == PHP
          ? cgi_bin_path = Configuration::getInstance().getPhpPath()
          : cgi_bin_path = Configuration::getInstance().getPythonPath();
      std::string method_str;
      switch (method_)
      {
        case GET:
          method_str = "GET";
          break;
        case POST:
          method_str = "POST";
          break;
        case DELETE:
          method_str = "DELETE";
          break;
        default:
          break;
      }
      response_ = new CgiResponse(fd_, closing_, cgi_bin_path, cgi_skript_path_,
                                  absolute_path_, method_str, query_string_,
                                  total_written_bytes_, POST);
      status_ = SENDING_RESPONSE;
      upload_file_.close();
      return;
    }
    std::map< std::string, std::string > additional_headers;
    additional_headers["Location"] = absolute_path_;
    if (!file_existed_)
      response_ =
          new StaticResponse(fd_, 201, closing_, "", additional_headers);
    else
      response_ =
          new StaticResponse(fd_, 200, closing_, "", additional_headers);
    upload_file_.close();
    if (current_upload_files_.erase(absolute_path_) == 0)
      throw RequestError(500, "File not found in current uploads");
    status_ = SENDING_RESPONSE;
  }
}
