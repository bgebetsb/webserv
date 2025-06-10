#pragma once

#include <string>

namespace Utils
{
  class FdWrap
  {
   public:
    FdWrap();
    ~FdWrap();

    void open(const char* filename, int flags);
    bool is_open(void) const;
    void close(void);
    bool bad(void) const;
    void write(const std::string& buffer);

   private:
    int fd_;
    bool bad_;

    FdWrap(const FdWrap& other);
    FdWrap& operator=(const FdWrap& other);
  };
}  // namespace Utils
