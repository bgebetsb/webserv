#include <stddef.h>
#include <sys/types.h>
#include <ctime>
#include <stdexcept>

namespace Utils
{
  /*
   * Returns the current time in milliseconds
   */
  u_int64_t getCurrentTime()
  {
    struct timespec time;
    u_int64_t milliseconds;

    if (clock_gettime(CLOCK_REALTIME, &time) == -1)
    {
      throw std::runtime_error("Error retrieving current time");
    }

    milliseconds = time.tv_sec * 1000 + time.tv_nsec / 1000000;

    return (milliseconds);
  }
}  // namespace Utils
