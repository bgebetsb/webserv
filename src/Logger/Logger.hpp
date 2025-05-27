#pragma once

#include <fstream>
#include <ostream>
#include <string>

/*
 * If there is no access_log configured, it will be in state `LogFile` but the
 * file will actually be `/dev/null`
 */
enum LogMode
{
  LOGFILE,
  STDOUT,
  STDERR
};

class Logger
{
 public:
  Logger();
  ~Logger();

  static std::ostream& log();
  static void openFile(const std::string& filename);
  static void setLogMode(LogMode mode);

 private:
  LogMode mode_;
  std::ofstream logfile_;

  Logger(const Logger& other);
  Logger& operator=(const Logger& other);

  static Logger& instance();
  std::ostream& getOstream();
  static std::string getCurrentTimestamp();
};
