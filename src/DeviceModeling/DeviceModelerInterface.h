/**
 * @file DeviceModelerInterface.h
 * @author Manadher Kharroubi (Manadher.Kharroubi@rapidsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Model.h"

class DeviceModelerInterface {
    public:
    DeviceModelerInterface() = default;

    bool device_name(int argc, const char **argv);
    bool device_version(int argc, const char **argv);
    bool schema_version(int argc, const char **argv);
    bool define_enum_type(int argc, const char **argv);
    bool define_block(int argc, const char **argv);
    bool define_ports(int argc, const char **argv);
    bool define_param_type(int argc, const char **argv);
    bool define_param(int argc, const char **argv);
    bool define_attr(int argc, const char **argv);
    bool define_constraint(int argc, const char **argv);
    bool create_instance(int argc, const char **argv);
};

