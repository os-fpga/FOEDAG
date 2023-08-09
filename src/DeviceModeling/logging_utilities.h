/**
 * @file logging_utilities.h
 * @author Manadher Kharroubi (manadher@rapidsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-18
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once
#include <spdlog/sinks/null_sink.h>

#define NULL_LOG                                                                 \
    do                                                                           \
    {                                                                            \
        auto buf_sink = std::make_shared<spdlog::sinks::null_sink_mt>();         \
        auto logger = std::make_shared<spdlog::logger>("null_logger", buf_sink); \
        spdlog::set_default_logger(logger);                                      \
    } while (0)

#define DEFAULT_LOG                                           \
    do                                                        \
    {                                                         \
        spdlog::set_default_logger(spdlog::default_logger()); \
    } while (0)
    