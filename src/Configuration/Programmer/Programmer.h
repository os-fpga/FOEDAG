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

#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include <functional>
#include <map>
#include <tuple>
#include <map>

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"

namespace FOEDAG {

enum ProgrammerErrorCode {
  NoError = 0,
  InvalidArgument = -100,
  DeviceNotFound = -101,
  CableNotFound = -102,
  CableNotSupported = -104,
  NoSupportedTapFound = -105,
  FailedExecuteCommand = -106,
  FailedToParseOutput = -107,
  BitfileNotFound = -108,
  FailedToProgramFPGA = -109,
  OpenOCDExecutableNotFound = -110,
};

extern std::map<int, std::string> ErrorMessages;

struct TapInfo {
  int index;
  std::string tapName;
  bool enabled;
  uint32_t idCode;
  uint32_t expected;
  uint32_t irLen;
  uint32_t irCap;
  uint32_t irMask;
};

static const std::vector<TapInfo> supportedTAP{
    {0, "Gemini", true, 0x1000563d, 0x1000563d, 0x5, 0x1, 0x3}};

static const std::vector<std::tuple<uint16_t, uint16_t>>
    supportedCableVendorIdProductId{{0x0403, 0x6011}, {0x0403, 0x6010}};

struct Device {
  int index;
  std::string name;
  int flashSize;
  TapInfo tapInfo;
};

enum class TransportType : uint32_t { jtag = 1 };
struct Cable {
  uint16_t vendorId;
  uint16_t productId;
  uint8_t busAddr;
  uint8_t portAddr;
  uint8_t deviceAddr;
  uint16_t channel = 0;
  std::string serialNumber = "";
  std::string description = "";
  uint32_t speed = 1000;  // kHz
  TransportType transport = TransportType::jtag;
};

enum class ProgramFlashOperation : uint32_t {
  Erase = 1,
  BlankCheck = 2,
  Program = 4,
  Verify = 8
};
inline ProgramFlashOperation operator|(ProgramFlashOperation lhs,
                                       ProgramFlashOperation rhs) {
  using T = std::underlying_type_t<ProgramFlashOperation>;
  return static_cast<ProgramFlashOperation>(static_cast<T>(lhs) |
                                            static_cast<T>(rhs));
}

inline ProgramFlashOperation operator&(ProgramFlashOperation lhs,
                                       ProgramFlashOperation rhs) {
  using T = std::underlying_type_t<ProgramFlashOperation>;
  return static_cast<ProgramFlashOperation>(static_cast<T>(lhs) &
                                            static_cast<T>(rhs));
}
struct CfgStatus {
  bool cfgDone;
  bool cfgError;
};

using ProgressCallback = std::function<void(std::string)>;
using OutputMessageCallback = std::function<void(std::string)>;

void programmer_entry(const CFGCommon_ARG* cmdarg);

// Backend API
/**
 * Initializes the programming library with the path to the OpenOCD executable.
 *
 * @param openOCDPath The path to the OpenOCD executable.
 * @return 0 if the library was initialized successfully, or a non-zero error
 * code otherwise.
 * @Note This function must be called before any other function in the library.
 */
int InitLibrary(std::string openOCDPath);

/**
 * Returns a string containing the error message for the given error code.
 *
 * @param errorCode The error code to get the message for.
 * @return A string containing the error message.
 */
std::string GetErrorMessage(int errorCode);

/**
 * Returns a vector containing the available cables that is connected on the
 * host machine.
 *
 * @param cables A vector to store the available cables in.
 * @return 0 if the cables were retrieved successfully, or a non-zero error code
 * otherwise.
 */
int GetAvailableCables(std::vector<Cable>& cables);

/**
 * Lists all devices that are connected to the specified USB cable.
 *
 * @param cable The `Cable` object that represents the USB cable to scan.
 * @param devices A reference to a vector of `Device` objects that will be
 * populated with information about the connected devices.
 *
 * @return An integer error code. If the function succeeds, the error code is
 * `ProgrammerErrorCode::NoError`. If an error occurs, the error code indicates
 * the type of error that occurred.
 */
int ListDevices(const Cable& cable, std::vector<Device>& devices);

/**
 * Gets the FPGA configuration status for the specified cable and device.
 *
 * @param cable The `Cable` object that represents the USB cable that connected
 * to the device.
 * @param device The target to FPGA device which you want to get its
 * configuration status.
 * @param status A reference to a CfgStatus object to store the configuration
 * status in.
 * @return 0 if the configuration status was retrieved successfully, or a
 * non-zero error code otherwise.
 */
int GetFpgaStatus(const Cable& cable, const Device& device, CfgStatus& status);

/**
 * Programs the specified FPGA with the given bitfile using the specified cable
 * and device.
 *
 * @param cable The cable to use for programming.
 * @param device The target device to program.
 * @param bitfile The path to the bitfile to program the FPGA.
 * @param stop An atomic boolean flag that can be used to stop the programming
 * process.
 * @param outStream An optional output stream to write progress messages to.
 * @param callbackMsg An optional callback function to allow caller to receive
 * output messages.
 * @param callbackProgress An optional callback function to allow caller to
 * receive progress updates.
 * @return 0 if the FPGA was programmed successfully, or a non-zero error code
 * otherwise.
 * @note The `callbackMsg` function is called with progress messages during the
 * programming operation. The `callbackProgress` function is called with
 * progress updates during the programming operation. Both functions are
 * optional and can be set to `nullptr` if not needed. The definition of the
 * callback functions are as follows:
 *   using ProgressCallback = std::function<void(std::string)>
 *   using OutputMessageCallback = std::function<void(std::string)>
 */
int ProgramFpga(const Cable& cable, const Device& device,
                const std::string& bitfile, std::atomic<bool>& stop,
                std::ostream* outStream = nullptr,
                OutputMessageCallback callbackMsg = nullptr,
                ProgressCallback callbackProgress = nullptr);

/**
 * Programs the specified flash memory with the given bitfile using the
 * specified cable and device.
 *
 * @param cable The cable to use for programming.
 * @param device The target device to program.
 * @param bitfile The path to the bitfile to program the flash memory with.
 * @param stop An atomic boolean flag that can be used to stop the programming
 * process.
 * @param modes The programming modes to use (default is erase and program).
 * @param outStream An optional output stream to write progress messages to.
 * @param callbackMsg An optional callback function to allow caller to receive
 * output messages.
 * @param callbackProgress An optional callback function to allow caller to
 * receive progress updates.
 * @return 0 if the flash memory was programmed successfully, or a non-zero
 * error code otherwise.
 *
 * @note The `modes` parameter is a bitfield that can be used to specify the
 * programming modes to use. The supported modes are
 * `ProgramFlashOperation::Erase`, `ProgramFlashOperation::BlankCheck`,
 * `ProgramFlashOperation::Program` and `ProgramFlashOperation::Verify`. The
 * default is to erase and program. The `callbackMsg` function is called with
 * messages during the programming operation. The `callbackProgress` function is
 * called with progress updates during the programming operation. Both functions
 * are optional and can be set to `nullptr` if not needed. The definition of the
 * callback functions are as follows:
 *   using ProgressCallback = std::function<void(std::string)>
 *   using OutputMessageCallback = std::function<void(std::string)>
 */
int ProgramFlash(const Cable& cable, const Device& device,
                 const std::string& bitfile, std::atomic<bool>& stop,
                 ProgramFlashOperation modes = (ProgramFlashOperation::Erase |
                                                ProgramFlashOperation::Program),
                 std::ostream* outStream = nullptr,
                 OutputMessageCallback callbackMsg = nullptr,
                 ProgressCallback callbackProgress = nullptr);

}  // namespace FOEDAG

#endif
