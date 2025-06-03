#include "PipeFd.hpp"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include "../exceptions/RequestError.hpp"
#include "epoll/EpollAction.hpp"
#include "exceptions/ExitExc.hpp"
#include "responses/CgiResponse.hpp"
#include "utils/Utils.hpp"

PipeFd::PipeFd(std::string& write_buffer,
               const std::string& skript_path,
               const std::string& cgi_path,
               const std::string& file_path,
               Response* cgi_response,
               char** envp)
    : EpollFd(),
      read_end_(-1),
      write_end_(-1),
      process_finished_(false),
      write_buffer_(write_buffer),
      bin_path_(cgi_path),
      skript_path_(skript_path),
      file_path_(file_path),
      cgi_response_(cgi_response),
      start_time_(Utils::getCurrentTime())
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
  // if (fcntl(fds[1], F_SETFL, O_NONBLOCK))
  // {
  //   closePipe();
  //   throw(RequestError(500, "fcntl failed on write end of pipe"));
  // }

  process_id_ = fork();
  if (process_id_ == -1)
  {
    closePipe();
    throw(RequestError(500, "Fork failed"));
  }
  if (process_id_ == 0)
  {
    close(fds[0]);
    fds[0] = -1;
    spawnCGI(envp);
    throw ExitExc();
  }
  else
  {
    close(fds[1]);
    fds[1] = -1;
  }
  fd_ = read_end_;
  if (waitpid(process_id_, NULL, WNOHANG) == process_id_)
  {
    closePipe();
    throw(RequestError(500, "CGI process finished before reading started"));
  }
  else
  {}
}

PipeFd::~PipeFd() {}

void PipeFd::closePipe()
{
  if (read_end_ != -1)
  {
    close(read_end_);
    read_end_ = -1;
  }
  if (write_end_ != -1)
  {
    close(write_end_);
    write_end_ = -1;
  }
}

void PipeFd::spawnCGI(char** envp)
{
  char* argv[3];
  argv[0] = const_cast< char* >(bin_path_.c_str());
  argv[1] = const_cast< char* >(skript_path_.c_str());
  argv[2] = NULL;

  int file_fd = open(file_path_.c_str(), O_RDONLY);
  if (file_fd == -1)
  {
    close(write_end_);
    write_end_ = -1;
    throw(ExitExc());
  }
  if (dup2(file_fd, STDIN_FILENO) == -1)
  {
    close(file_fd);
    close(write_end_);
    write_end_ = -1;
    throw(ExitExc());
  }
  if (dup2(write_end_, STDOUT_FILENO) == -1)
  {
    close(file_fd);
    close(write_end_);
    write_end_ = -1;
    throw(ExitExc());
  }
  for (int i = 0; argv[i]; i++)
  {
    std::cerr << "argv[" << i << "] = " << argv[i] << std::endl;
  }

  for (int i = 0; envp[i]; i++)
  {
    std::cerr << "envp[" << i << "] = " << envp[i] << std::endl;
  }
  execve(bin_path_.c_str(), argv, envp);
  close(write_end_);
  write_end_ = -1;
}

EpollAction PipeFd::epollCallback(int event)
{
  std::cout << "PipeFd::epollCallback called for fd: " << fd_ << std::endl;
  EpollAction action = {fd_, EPOLL_ACTION_UNCHANGED, NULL};

  CgiResponse* response = dynamic_cast< CgiResponse* >(cgi_response_);

  if (event & EPOLLIN)
  {
    ssize_t bytes_read_ = read(read_end_, read_buffer_, CHUNK_SIZE);
    std::cout << "PipeFd::epollCallback read " << bytes_read_
              << " bytes from pipe" << std::endl;
    if (bytes_read_ == -1)
    {
      (void)response;
      process_finished_ = true;
      kill(process_id_, SIGKILL);
      response->setCloseConnectionHeader();
      return action;  // Error reading from pipe
    }
    write_buffer_.append(read_buffer_, bytes_read_);
    int status_code;
    if (waitpid(process_id_, &status_code, WNOHANG) == process_id_)
    {
      if (WIFEXITED(status_code))
      {
        process_finished_ = true;
        if (WEXITSTATUS(status_code) != 0)
          response->setCloseConnectionHeader();
      }
      else
      {
        process_finished_ = true;
        kill(process_id_, SIGKILL);
        response->setCloseConnectionHeader();
      }
    }
  }
  else if (event & EPOLLHUP)
  {
    if (!process_finished_)
    {
      process_finished_ = true;
      kill(process_id_, SIGKILL);
      response->setCloseConnectionHeader();
    }
    else
    {
      process_finished_ = true;
      kill(process_id_, SIGKILL);
      response->setCloseConnectionHeader();
    }
  }
  else
  {
    std::cout << "PipeFd::epollCallback called for fd: " << fd_
              << " but no EPOLLIN event(" << event << ")" << std::endl;
  }
  return action;
}
