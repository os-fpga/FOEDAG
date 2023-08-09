/**
 * @file Model.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-26
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