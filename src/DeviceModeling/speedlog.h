/**
 * @file speedlog.h
 * @author Manadher Kharroubi (manadher@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-05-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef SPEEDLOG_H
#define SPEEDLOG_H

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#ifndef spdlog
#define spdlog SpeedLog
#endif

enum LogLevel { LOG_INFO, LOG_WARN, LOG_ERROR };

class SpeedLog {
 public:
  static void setLogLevel(LogLevel level) { speed_logLevel = level; }

  template <typename... Args>
  static void log(LogLevel level, const std::string& format, Args&&... args) {
    if (level >= speed_logLevel) {
      std::stringstream ss;
      std::string formatCopy = format;  // Make a copy for modification
      logInternal(ss, level, formatCopy, std::forward<Args>(args)...);
      std::cout << ss.str();
    }
  }

  template <typename... Args>
  static void info(const std::string& format, Args&&... args) {
    log(LOG_INFO, format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void warn(const std::string& format, Args&&... args) {
    log(LOG_WARN, format, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void error(const std::string& format, Args&&... args) {
    log(LOG_ERROR, format, std::forward<Args>(args)...);
  }

 private:
  static LogLevel speed_logLevel;

  template <typename T, typename... Args>
  static void logValue(std::stringstream& ss, LogLevel level,
                       std::string& format, const T& value,
                       const Args&... args) {
    //   while (!format.empty()) {
    //       if (format[0] == '{' && format[1] == '}') {
    //           ss << value;
    //           format = format.substr(2);
    //           logValueHelper(ss, level, format, args...);
    //           return;
    //       }
    //       ss << format[0];
    //       format = format.substr(1);
    //   }
    ss << format << " ";
    ((ss << args << " "), ...);  // Using the fold expression and comma operator
    ss << std::endl;
  }

  //   template <typename T>
  //   static void logValueHelper(std::stringstream& ss, LogLevel level,
  //   std::string& format, const T& value) {
  //       ss << value;
  //       logValue(ss, level, format);
  //   }

  //   template <typename T, typename... Args>
  //   static void logValueHelper(std::stringstream& ss, LogLevel level,
  //   std::string& format, const T& value, const Args&... args) {
  //       ss << value;
  //       logValue(ss, level, format, args...);
  //   }

  template <typename... Args>
  static void logInternal(std::stringstream& ss, LogLevel level,
                          std::string& format, const Args&... args) {
    ss << format << " ";
    ((ss << args << " "), ...);  // Using the fold expression and comma operator
    ss << std::endl;
    //  logValue(ss, level, format, args...);
  }
};

#endif  // SPEEDLOG_H
