/**
 * @file Model.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-26
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "Model.h"

device_modeler& Model::get_modler() { return device_modeler::instance(); }