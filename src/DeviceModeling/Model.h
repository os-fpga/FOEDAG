/**
 * @file Model.h
 * @author your name (manadher@gmail.com)
 * @brief
 * @version 1.0
 * @date 2024-06-7
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#if (defined(_MSC_VER) || defined(__CYGWIN__))
#define NOMINMAX  // prevent error with std::max
#endif

#include "device_modeler.h"

class Model {
 public:
  static device_modeler& get_modler();
};