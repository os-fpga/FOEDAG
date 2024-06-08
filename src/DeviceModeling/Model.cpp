/**
 * @file Model.cpp
 * @author your name (manadher@gmail.com)
 * @brief
 * @version 1.0
 * @date 2024-06-7
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "Model.h"

device_modeler& Model::get_modler() { return device_modeler::instance(); }