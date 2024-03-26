/**
  * @file CommConstants.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COMMCONSTS_H
#define COMMCONSTS_H

namespace FOEDAG {
namespace comm {

const int CRITICAL_PATH_NUM_THRESHOLD =
    10000;  // there is no sence practially to use value bigger than 10000, and
            // more over bigger value may produce allocation errors due to PC
            // limitation and memory fragmentation

constexpr const char* KEY_JOB_ID = "JOB_ID";
constexpr const char* KEY_CMD = "CMD";
constexpr const char* KEY_OPTIONS = "OPTIONS";
constexpr const char* KEY_DATA = "DATA";
constexpr const char* KEY_STATUS = "STATUS";
constexpr const char* ECHO_DATA = "ECHO";

const unsigned char ZLIB_COMPRESSOR_ID = 'z';
const unsigned char NONE_COMPRESSOR_ID = '\x0';

constexpr const char* OPTION_PATH_NUM = "path_num";
constexpr const char* OPTION_PATH_TYPE = "path_type";
constexpr const char* OPTION_DETAILS_LEVEL = "details_level";
constexpr const char* OPTION_IS_FLOAT_ROUTING = "is_flat_routing";
constexpr const char* OPTION_PATH_ELEMENTS = "path_elements";
constexpr const char* OPTION_HIGHTLIGHT_MODE = "hight_light_mode";
constexpr const char* OPTION_DRAW_PATH_CONTOUR = "draw_path_contour";

constexpr const char* CRITICAL_PATH_ITEMS_SELECTION_NONE = "none";

// please don't change values as they are involved in socket communication
constexpr const char* KEY_SETUP_PATH_LIST = "setup";
constexpr const char* KEY_HOLD_PATH_LIST = "hold";
//

enum CMD { CMD_GET_PATH_LIST_ID = 0, CMD_DRAW_PATH_ID };

}  // namespace comm

}  // namespace FOEDAG

#endif
