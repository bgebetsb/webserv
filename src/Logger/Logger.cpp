#include "Logger.hpp"
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include "../exceptions/Fatal.hpp"

using std::ios_base;

Logger::Logger() : mode_(LOGFILE), logfile_("/dev/null") {}

Logger::~Logger() {}

Logger& Logger::instance()
{
  static Logger logger;

  return logger;
}

std::ostream& Logger::getOstream()
{
  switch (mode_)
  {
    case LOGFILE:
      if (!logfile_.is_open())
        return std::cerr;
      return logfile_;
    case STDOUT:
      return std::cout;
    default:
      return std::cerr;
  }
}

std::ostream& Logger::log()
{
  Logger& logger = Logger::instance();

  std::ostream& stream = logger.getOstream();
  stream << Logger::getCurrentTimestamp() << " ";

  return stream;
}

void Logger::openFile(const std::string& filename)
{
  Logger& logger = Logger::instance();

  if (logger.logfile_.is_open())
    logger.logfile_.close();

  logger.logfile_.open(filename.c_str(), ios_base::out | ios_base::app);
  if (!logger.logfile_.is_open())
    throw Fatal("Unable to open logfile " + filename);
}

void Logger::setLogMode(LogMode mode)
{
  Logger& logger = Logger::instance();

  if (mode != LOGFILE && logger.logfile_.is_open())
    logger.logfile_.close();

  logger.mode_ = mode;
}

std::string Logger::getCurrentTimestamp()
{
  std::time_t current = std::time(NULL);

  if (current == static_cast< std::time_t >(-1))
    throw std::runtime_error("Unable to get current timestamp");

  tm* local = std::localtime(&current);
  if (!local)
    throw std::runtime_error("Unable to convert timestamp to localtime");

  char buffer[22];
  std::strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", local);

  return std::string(buffer);
}

void Logger::close()
{
  Logger& logger = Logger::instance();

  if (logger.logfile_.is_open())
    logger.logfile_.close();
}
