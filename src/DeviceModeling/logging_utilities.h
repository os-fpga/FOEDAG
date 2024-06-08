/**
 * @file logging_utilities.h
 * @author Manadher Kharroubi (manadher@gmail.com)
 * @brief
 * @version 1.0
 * @date 2024-06-7
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once
#include <spdlog/sinks/null_sink.h>

#define NULL_LOG                                                             \
  do {                                                                       \
    auto buf_sink = std::make_shared<spdlog::sinks::null_sink_mt>();         \
    auto logger = std::make_shared<spdlog::logger>("null_logger", buf_sink); \
    spdlog::set_default_logger(logger);                                      \
  } while (0)

#define DEFAULT_LOG                                       \
  do {                                                    \
    spdlog::set_default_logger(spdlog::default_logger()); \
  } while (0)
