#include "FdWrap.hpp"
#include <fcntl.h>
#include <unistd.h>

namespace Utils
{
  FdWrap::FdWrap() : fd_(-1), bad_(false) {}

  FdWrap::~FdWrap()
  {
    if (fd_ != -1)
      ::close(fd_);
  }

  void FdWrap::open(const char* filename, int flags)
  {
    fd_ = ::open(filename, flags, 0600);
  }

  bool FdWrap::is_open(void) const
  {
    return (fd_ != -1);
  }

  bool FdWrap::bad(void) const
  {
    return bad_;
  }

  void FdWrap::close(void)
  {
    if (fd_ != -1)
    {
      ::close(fd_);
      fd_ = -1;
    }
  }

  void FdWrap::write(const std::string& buffer)
  {
    ssize_t ret = ::write(fd_, buffer.c_str(), buffer.size());

    if (ret == -1)
      bad_ = true;
  }
}  // namespace Utils
