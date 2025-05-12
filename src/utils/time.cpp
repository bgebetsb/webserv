#include <stddef.h>
#include <ctime>
#include <stdexcept>

namespace Utils
{
  /*
   * Returns the current time in seconds
   */
  size_t getCurrentTime()
  {
    struct timespec time;
    if (clock_gettime(CLOCK_REALTIME, &time) == -1)
    {
      throw std::runtime_error("Error retrieving current time");
    }

    return (time.tv_sec);
  }
}  // namespace Utils
