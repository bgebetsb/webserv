#include "PipeFd.hpp"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../exceptions/RequestError.hpp"
#include "exceptions/ExitExc.hpp"

PipeFd::PipeFd(std::string& write_buffer,
               std::string& skript_path,
               std::string& cgi_path,
               std::string& file_path,
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
      cgi_response_(cgi_response)
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
  if (fcntl(fds[1], F_SETFL, O_NONBLOCK))
  {
    closePipe();
    throw(RequestError(500, "fcntl failed on write end of pipe"));
  }

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
  if (waitpid(process_id_, NULL, WNOHANG) == process_id_)
  {
    closePipe();
    throw(RequestError(500, "CGI process finished before reading started"));
  }
  else
  {
    fd_ = read_end_;
  }
}

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
  execve(bin_path_.c_str(), argv, envp);
  close(write_end_);
  write_end_ = -1;
}
