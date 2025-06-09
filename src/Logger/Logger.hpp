#pragma once

#include <fstream>
#include <ostream>
#include <string>
#include "Configs/Configs.hpp"

class Logger
{
 public:
  Logger();
  ~Logger();

  static std::ostream& log();
  static void openFile(const std::string& filename);
  static void setLogMode(LogMode mode);
  static void close();

 private:
  LogMode mode_;
  std::ofstream logfile_;

  Logger(const Logger& other);
  Logger& operator=(const Logger& other);

  static Logger& instance();
  std::ostream& getOstream();
  static std::string getCurrentTimestamp();
};
