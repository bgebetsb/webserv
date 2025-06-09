#include <stddef.h>
#include <sys/types.h>
#include <ctime>
#include <stdexcept>

namespace Utils
{
  /*
   * Returns the current time in seconds
   */
  u_int64_t getCurrentTime()
  {
    std::time_t time = std::time(NULL);

    if (time == static_cast< std::time_t >(-1))
      throw std::runtime_error("Error retrieving current time");

    return (static_cast< u_int64_t >(time));
  }
}  // namespace Utils
