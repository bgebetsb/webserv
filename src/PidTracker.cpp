#include "PidTracker.hpp"
#include <sys/wait.h>
#include <cstddef>

KillStatus::KillStatus() : seconds(0), sigkill(false) {}

PidTracker::PidTracker() {}

PidTracker::~PidTracker()
{
  MPid::iterator it;

  for (it = pids_.begin(); it != pids_.end(); ++it)
  {
    kill(it->first, SIGKILL);
    waitpid(it->first, NULL, 0);
  }
}

void PidTracker::killPid(pid_t pid)
{
  if (pids_.find(pid) != pids_.end())
  {
    kill(pid, SIGTERM);
    pids_[pid] = KillStatus();
  }
}

void PidTracker::ping()
{
  MPid::iterator it = pids_.begin();

  while (it != pids_.end())
  {
    if (waitpid(it->first, NULL, WNOHANG) != 0)
      pids_.erase(it++);
    else
    {
      it->second.seconds++;
      if (it->second.seconds > 5 && !it->second.sigkill)
      {
        kill(it->first, SIGKILL);
        it->second.sigkill = true;
      }
      ++it;
    }
  }
}

PidTracker& getPidTracker(void)
{
  static PidTracker pidtracker;

  return pidtracker;
}
