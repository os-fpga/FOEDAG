/*
Copyright 2023 The Foedag team

GPL License

Copyright (c) 2023 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __OPENOCDADAPTER_H__
#define __OPENOCDADAPTER_H__

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "Configuration/Programmer/CfgStatus.h"
#include "JtagAdapter.h"
#include "ProgrammingAdapter.h"
#include "Tap.h"
namespace FOEDAG {

enum CommandOutputType {
  NOT_OUTPUT = 0,
  CMD_PROGRESS,
  CMD_ERROR,
  CMD_TIMEOUT,
  CBUFFER_TIMEOUT,
  CONFIG_ERROR,
  CONFIG_SUCCESS,
  UNKNOWN_FIRMWARE,
  FSBL_BOOT_FAILURE,
  INVALID_BITSTREAM,
};
using ProgressCallback = std::function<void(std::string)>;
using OutputMessageCallback = std::function<void(std::string)>;

class OpenocdAdapter : public JtagAdapter, public ProgrammingAdapter {
 public:
  OpenocdAdapter(std::string openocd);
  virtual ~OpenocdAdapter();
  virtual std::vector<uint32_t> scan(const Cable& cable) override;

  int program_fpga(const Device& device, const std::string& bitfile,
                   std::atomic<bool>& stop, std::ostream* outStream = nullptr,
                   OutputMessageCallback callbackMsg = nullptr,
                   ProgressCallback callbackProgress = nullptr) override;
  int program_flash(const Device& device, const std::string& bitfile,
                    std::atomic<bool>& stop, ProgramFlashOperation modes,
                    std::ostream* outStream = nullptr,
                    OutputMessageCallback callbackMsg = nullptr,
                    ProgressCallback callbackProgress = nullptr) override;
  int program_otp(const Device& device, const std::string& bitfile,
                  std::atomic<bool>& stop, std::ostream* outStream = nullptr,
                  OutputMessageCallback callbackMsg = nullptr,
                  ProgressCallback callbackProgress = nullptr) override;
  int query_fpga_status(const Device& device, CfgStatus& cfgStatus,
                        std::string& outputMessage) override;

  static CommandOutputType check_output(std::string str,
                                        std::vector<std::string>& output);
  static bool check_regex(std::string str, std::string pattern,
                          std::vector<std::string>& output);
  std::string get_last_output() { return m_last_output; };

  void update_taplist(const std::vector<Tap>& taplist);

 private:
  int execute(const Cable& cable, std::string cmd, std::string& output);
  std::string m_openocd;
  std::vector<Tap> m_taplist;
  std::string m_last_output;
};

}  // namespace FOEDAG

#endif  //__OPENOCDADAPTER_H__