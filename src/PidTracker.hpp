#pragma once

#include <sys/types.h>
#include <map>

struct KillStatus
{
  unsigned int seconds;
  bool sigkill;

  KillStatus();
};

typedef std::map< pid_t, KillStatus > MPid;

class PidTracker
{
 public:
  PidTracker();
  ~PidTracker();

  void killPid(pid_t pid);
  void ping();

 private:
  MPid pids_;

  PidTracker(const PidTracker& other);
  PidTracker& operator=(const PidTracker& other);
};

PidTracker& getPidTracker();
