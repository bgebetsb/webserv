#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <csignal>
#include <cstddef>
#include <exception>
#include <iostream>
#include "Webserv.hpp"
#include "exceptions/ExitExc.hpp"

volatile sig_atomic_t g_signal = 0;

void handle_signal(int signum)
{
  g_signal = signum;
}

void setup_signals(void)
{
  int catch_signals[] = {SIGINT, SIGQUIT, SIGHUP, SIGTERM, 0};
  int ignore_signals[] = {SIGUSR1, SIGUSR2, 0};

  for (size_t i = 0; catch_signals[i]; ++i)
  {
    signal(catch_signals[i], handle_signal);
  }

  for (size_t i = 0; ignore_signals[i]; ++i)
  {
    signal(ignore_signals[i], SIG_IGN);
  }
}

int main(int argc, char* argv[])
try
{
  setup_signals();
  try
  {
    if (argc > 2)
    {
      std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
      return 1;
    }
    else if (argc == 2)
    {
      Webserv w(argv[1]);
      w.mainLoop();
    }
    else
    {
      Webserv w;
      w.mainLoop();
    }
  }
  catch (ExitExc& e)
  {
    std::cerr << "Error in CGI Process!" << std::endl;
    return (EXITERR);
  }
  catch (std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return (1);
  }
}
catch (...)
{}
