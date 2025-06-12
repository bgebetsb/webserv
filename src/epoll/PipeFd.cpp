#include "PipeFd.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstddef>
#include <iostream>
#include <sstream>
#include "../exceptions/RequestError.hpp"
#include "Configs/Configs.hpp"
#include "Logger/Logger.hpp"
#include "PidTracker.hpp"
#include "epoll/EpollAction.hpp"
#include "epoll/EpollData.hpp"
#include "exceptions/ExitExc.hpp"
#include "requests/RequestMethods.hpp"
#include "responses/CgiResponse.hpp"
#include "utils/Utils.hpp"
PipeFd::PipeFd(std::string& write_buffer,
               const std::string& skript_path,
               const std::string& cgi_path,
               const std::string& file_path,
               Response* cgi_response,
               char** envp,
               RequestMethod method)
    : EpollFd(),
      read_end_(-1),
      write_end_(-1),
      process_finished_(false),
      write_buffer_(write_buffer),
      bin_path_(cgi_path),
      skript_path_(skript_path),
      file_path_(file_path),
      cgi_response_(cgi_response),
      start_time_(Utils::getCurrentTime()),
      method_(method)
{
  int fds[2];
  if (pipe(fds) == -1)
  {
    throw RequestError(500, "Pipe creation failed");
  }
  write_end_ = fds[1];
  read_end_ = fds[0];
  if (fcntl(fds[0], F_SETFL, O_NONBLOCK))
  {
    closePipe();
    throw(RequestError(500, "fcntl failed on read end of pipe"));
  }

  process_id_ = fork();
  if (process_id_ == -1)
  {
    closePipe();
    throw(RequestError(500, "Fork failed"));
  }
  else if (process_id_ == 0)
  {
    Utils::ft_close(fds[0]);
    Logger::close();  // Needs to be closed manually because we can't set
                      // O_CLOEXEC on ofstreams...
    Configuration& config = Configuration::getInstance();
    const LogSettings& error_settings = config.getErrorLogsettings();
    if (error_settings.configured)
    {
      if (error_settings.mode == LOGFILE)
      {
        int fd = open(error_settings.logfile.c_str(),
                      O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (fd == -1)
          std::cerr << "WARNING: Unable to open logfile for error log"
                    << std::endl;
        else
        {
          dup2(fd, STDERR_FILENO);
          close(fd);
        }
      }
    }
    else
    {
      close(STDERR_FILENO);
    }
    spawnCGI(envp);
    // Close stdin/stdout in child after execve failure, because they're fds
    // associated with files now
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    if (!error_settings.configured)
      close(STDERR_FILENO);
    throw ExitExc();
  }
  else
  {
    Utils::ft_close(fds[1]);

    EpollData& ed = getEpollData();

    if (Utils::addCloExecFlag(fds[0]) == -1 ||
        epoll_ctl(ed.fd, EPOLL_CTL_ADD, read_end_, getEvent()) == -1)
    {
      Utils::ft_close(fds[0]);
      killProcess();
      throw RequestError(500, "Unable to add FD of pipe to epoll");
    }
    ed.fds[read_end_] = this;
  }
  fd_ = read_end_;
}

PipeFd::~PipeFd()
{
  if (cgi_response_)
  {
    CgiResponse* converted = reinterpret_cast< CgiResponse* >(cgi_response_);
    enableSending(converted);
    converted->unsetPipeFd();
  }

  killProcess();
}

void PipeFd::closePipe()
{
  Utils::ft_close(read_end_);
  Utils::ft_close(write_end_);
}

void PipeFd::spawnCGI(char** envp)
{
  char* argv[3];
  argv[0] = const_cast< char* >(bin_path_.c_str());
  argv[1] = const_cast< char* >(skript_path_.c_str());
  argv[2] = NULL;

  std::string directory =
      skript_path_.substr(0, skript_path_.find_last_of('/') + 1);
  if (chdir(directory.c_str()) == -1)
  {
    Utils::ft_close(write_end_);
    throw(RequestError(500, "chdir failed in CGI process"));
  }
  int file_fd = -1;
  if (!file_path_.empty() && method_ == POST)
  {
    file_fd = open(file_path_.c_str(), O_RDONLY);
    if (file_fd == -1)
    {
      Utils::ft_close(write_end_);
      throw(ExitExc());
    }
    if (dup2(file_fd, STDIN_FILENO) == -1)
    {
      Utils::ft_close(file_fd);
      Utils::ft_close(write_end_);
      throw(ExitExc());
    }
    Utils::ft_close(file_fd);
  }
  else
    close(STDIN_FILENO);

  if (dup2(write_end_, STDOUT_FILENO) == -1)
  {
    close(STDIN_FILENO);
    Utils::ft_close(write_end_);
    throw(ExitExc());
  }
  Utils::ft_close(write_end_);
  for (int i = 0; argv[i]; i++)
  {
    std::cerr << "argv[" << i << "] = " << argv[i] << std::endl;
  }

  for (int i = 0; envp[i]; i++)
  {
    std::cerr << "envp[" << i << "] = " << envp[i] << std::endl;
  }
  execve(bin_path_.c_str(), argv, envp);
}

EpollAction PipeFd::epollCallback(int event)
{
  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, NULL};

  CgiResponse* response = reinterpret_cast< CgiResponse* >(cgi_response_);

  if (!response)
  {
    killProcess();
    process_finished_ = true;
  }
  else if (event & EPOLLIN)
  {
    ssize_t bytes_read_ = read(read_end_, read_buffer_, CHUNK_SIZE);
    std::cout << "PipeFd::epollCallback read " << bytes_read_
              << " bytes from pipe" << std::endl;
    if (bytes_read_ == -1)
    {
      killProcess();
      process_finished_ = true;
      response->setCloseConnectionHeader();
    }
    else
    {
      if (bytes_read_ > 0)
      {
        if (response->getHeadersCreated())
        {
          std::ostringstream ss;
          ss << std::hex << bytes_read_ << "\r\n";
          write_buffer_.append(ss.str());
          write_buffer_.append(read_buffer_, bytes_read_);
          write_buffer_.append("\r\n");
        }
        else
        {
          write_buffer_.append(read_buffer_, bytes_read_);
        }
      }
      checkExited(response);
    }
  }
  else if (event & EPOLLHUP)
  {
    checkExited(response);
    if (!process_finished_)
    {
      killProcess();
      process_finished_ = true;
    }
  }
  else
  {
    std::cout << "PipeFd::epollCallback called for fd: " << fd_
              << " but no EPOLLIN event(" << event << ")" << std::endl;
  }

  enableSending(response);
  if (process_finished_)
    action.op = EPOLL_ACTION_DEL;
  return action;
}

void PipeFd::unsetResponse(void)
{
  cgi_response_ = NULL;
  killProcess();
}

void PipeFd::checkExited(CgiResponse* response)
{
  if (process_id_ == -1)
    return;

  int status_code;
  if (waitpid(process_id_, &status_code, WNOHANG) == process_id_)
  {
    if (WIFEXITED(status_code))
    {
      if (WEXITSTATUS(status_code) != 0)
        response->setCloseConnectionHeader();
      process_id_ = -1;
    }
    else if (WIFSIGNALED(status_code))
    {
      response->setCloseConnectionHeader();
      process_id_ = -1;
    }
    else
    {
      killProcess();
      response->setCloseConnectionHeader();
    }
  }
}

void PipeFd::killProcess()
{
  if (process_id_ != -1)
  {
    PidTracker& pidtracker = getPidTracker();
    pidtracker.killPid(process_id_);
  }
}

void PipeFd::enableSending(CgiResponse* response)
{
  if (response)
  {
    int fd = response->getClientFd();
    EpollData& ed = getEpollData();
    if (ed.fds.find(fd) != ed.fds.end())
    {
      epoll_event* event = ed.fds[fd]->getEvent();
      if (event->events == 0)
      {
        event->events = EPOLLOUT | EPOLLRDHUP;
        if (epoll_ctl(ed.fd, EPOLL_CTL_MOD, fd, event) == -1)
        {
          killProcess();
          process_finished_ = true;
        }
      }
    }
  }
}

size_t PipeFd::getStartTime() const
{
  return start_time_;
}

Response* PipeFd::getResponse() const
{
  return cgi_response_;
}
