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

#include "Programmer.h"

#include <iostream>
#include <sstream>  // for std::stringstream
#include <thread>
#include <unordered_set>

#include "Programmer_helper.h"
#include "libusb.h"

// temporary to suppress warning
// <TODO> remove this when implementing programmer tcl command
#define UNUSED(variable) ((void)variable)
namespace FOEDAG {

// openOCDPath used by library
static std::string libOpenOcdExecPath;
static std::vector<TapInfo> foundTap;

std::map<int, std::string> ErrorMessages = {
    {NoError, "Success"},
    {InvalidArgument, "Invalid argument"},
    {DeviceNotFound, "Device not found"},
    {CableNotFound, "Cable not found"},
    {CableNotSupported, "Cable not supported"},
    {NoSupportedTapFound, "No supported tap found"},
    {FailedExecuteCommand, "Failed to execute command"},
    {FailedToParseOutput, "Failed to parse output"},
    {BitfileNotFound, "Bitfile not found"},
    {FailedToProgramFPGA, "Failed to program FPGA"},
    {OpenOCDExecutableNotFound, "OpenOCD executable not found"},
};

void programmer_entry(const CFGCommon_ARG* cmdarg) {
  auto arg = std::static_pointer_cast<CFGArg_PROGRAMMER>(cmdarg->arg);
  if (arg == nullptr) return;

  if (arg->m_help) {
    return;
  }

  std::string subCmd = arg->get_sub_arg_name();
  if (cmdarg->compilerName == "dummy") {
    if (subCmd == "list_device") {
      CFG_POST_MSG("<test>      | Device             |  ID        |  IRLen ");
      CFG_POST_MSG("<test> ----- -------------------- ------------ ----------");
      CFG_POST_MSG("<test> Found  0 Gemini             0x1000AABB   5");
      CFG_POST_MSG("<test> Found  1 Gemini             0x2000CCDD   5");
    } else if (subCmd == "list_cable") {
      CFG_POST_MSG("<test>  1 Usb_Programmer_Cable_port1_dev1");
      CFG_POST_MSG("<test>  2 Usb_Programmer_Cable_port2_dev1");
    } else if (subCmd == "fpga_status") {
      CFG_POST_MSG("<test> FPGA configuration status : Done");
    } else if (subCmd == "fpga_config") {
      for (int i = 10; i <= 100; i += 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        CFG_POST_MSG("<test> program fpga - %d %%", i);
      }
    } else if (subCmd == "flash") {
      auto flash =
          static_cast<const CFGArg_PROGRAMMER_FLASH*>(arg->get_sub_arg());
      auto operations = parseOperationString(flash->operations);
      if (isOperationRequested("erase", operations)) {
        CFG_POST_MSG("<test> Erasing flash memory");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> erase flash - %d %% ", i);
        }
      }
      if (isOperationRequested("blankcheck", operations)) {
        CFG_POST_MSG("<test> Flash blank check start ...");
        CFG_POST_MSG("<test> Flash blank check complete.");
      }
      if (isOperationRequested("program", operations)) {
        CFG_POST_MSG("<test> Programming flash memory");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> program flash - %d %% ", i);
        }
      }
      if (isOperationRequested("verify", operations)) {
        CFG_POST_MSG("<test> Flash verification start ...");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> flash verified- %d %% ", i);
        }
      }
    }
  } else {
    // check openocd executable
    const std::filesystem::path openOcdExecPath = cmdarg->toolPath;
    std::error_code ec;
    if (!std::filesystem::exists(openOcdExecPath, ec)) {
      CFG_POST_ERR("Cannot find openocd executable: %s. %s. ",
                   openOcdExecPath.string().c_str(),
                   (ec ? ec.message().c_str() : ""));
      return;
    }
    InitLibrary(openOcdExecPath);
    if (subCmd == "list_device") {
      auto list_device =
          static_cast<const CFGArg_PROGRAMMER_LIST_DEVICE*>(arg->get_sub_arg());
      UNUSED(list_device);
      //<TODO> implement programmer tcl command based on programmer API lib
    } else if (subCmd == "list_cable") {
      auto list_cable =
          static_cast<const CFGArg_PROGRAMMER_LIST_CABLE*>(arg->get_sub_arg());
      UNUSED(list_cable);
      //<TODO> implement programmer tcl command based on programmer API lib
    } else if (subCmd == "fpga_status") {
      auto fpga_status =
          static_cast<const CFGArg_PROGRAMMER_FPGA_STATUS*>(arg->get_sub_arg());
      UNUSED(fpga_status);
      //<TODO> implement programmer tcl command based on programmer API lib
    } else if (subCmd == "fpga_config") {
      auto fpga_config =
          static_cast<const CFGArg_PROGRAMMER_FPGA_CONFIG*>(arg->get_sub_arg());
      UNUSED(fpga_config);
      //<TODO> implement programmer tcl command based on programmer API lib
    } else if (subCmd == "flash") {
      auto flash =
          static_cast<const CFGArg_PROGRAMMER_FLASH*>(arg->get_sub_arg());
      auto operations = parseOperationString(flash->operations);
      UNUSED(operations);
      //<TODO> implement programmer tcl command based on programmer API lib
    }
  }
}

int InitLibrary(std::string openOCDPath) {
  CFG_ASSERT_MSG(!openOCDPath.empty(), "openOCDPath cannot be empty");
  libOpenOcdExecPath = openOCDPath;

  if (!std::filesystem::exists(openOCDPath)) {
    return ProgrammerErrorCode::OpenOCDExecutableNotFound;
  }

  return ProgrammerErrorCode::NoError;
}

std::string GetErrorMessage(int errorCode) {
  auto it = ErrorMessages.find(errorCode);
  if (it != ErrorMessages.end()) {
    return it->second;
  }
  return "Unknown error.";
}

int GetAvailableCables(std::vector<Cable>& cables) {
  struct libusb_context* jtagLibusbContext = nullptr; /**< Libusb context **/
  struct libusb_device** deviceList = nullptr; /**< The usb device list **/
  struct libusb_device_handle* libusbHandle = nullptr;
  std::string outputMsg;
  cables.clear();
  outputMsg.clear();
  int deviceCount = 0;
  int returnCode = 0;

  returnCode = libusb_init(&jtagLibusbContext);
  if (returnCode < 0) {
    outputMsg = "libusb_init() failed with " +
                std::string(libusb_error_name(returnCode)) + "\n";
    outputMsg += "GetAvailableCables() failed.\n";
    addOrUpdateErrorMessage(returnCode, outputMsg);
    return returnCode;
  }

  deviceCount = libusb_get_device_list(jtagLibusbContext, &deviceList);
  for (int index = 0; index < deviceCount; index++) {
    struct libusb_device_descriptor devDesc;

    if (libusb_get_device_descriptor(deviceList[index], &devDesc) != 0) {
      continue;
    }

    for (size_t i = 0; i < supportedCableVendorIdProductId.size(); i++) {
      if (devDesc.idVendor == std::get<0>(supportedCableVendorIdProductId[i]) &&
          devDesc.idProduct ==
              std::get<1>(supportedCableVendorIdProductId[i])) {
        Cable cable;
        cable.vendorId = devDesc.idVendor;
        cable.productId = devDesc.idProduct;
        cable.portAddr = libusb_get_port_number(deviceList[index]);
        cable.deviceAddr = libusb_get_device_address(deviceList[index]);
        cable.busAddr = libusb_get_bus_number(deviceList[index]);

        returnCode = libusb_open(deviceList[index], &libusbHandle);

        if (returnCode) {
          outputMsg += "libusb_open() failed with " +
                       std::string(libusb_error_name(returnCode)) + "\n";
          outputMsg += "GetAvailableCables() failed.\n";
          addOrUpdateErrorMessage(returnCode, outputMsg);
          continue;
        }
        returnCode = get_string_descriptor(libusbHandle, devDesc.iProduct,
                                           cable.description, outputMsg);
        if (returnCode < 0) {
          addOrUpdateErrorMessage(returnCode, outputMsg);
          libusb_close(libusbHandle);
          continue;
        }

        if (get_string_descriptor(libusbHandle, devDesc.iSerialNumber,
                                  cable.serialNumber, outputMsg) < 0) {
          cable.serialNumber = "";  // ignore error, not all usb cable has
                                    // serial number
        }

        cables.push_back(cable);
        libusb_close(libusbHandle);
      }
    }
  }

  if (deviceList != nullptr) libusb_free_device_list(deviceList, 1);

  if (jtagLibusbContext != nullptr) libusb_exit(jtagLibusbContext);

  return ProgrammerErrorCode::NoError;
}

int ListDevices(const Cable& cable, std::vector<Device>& devices) {
  CFG_ASSERT_MSG(!libOpenOcdExecPath.empty(),
                 "libOpenOcdExecPath cannot be empty");
  int returnCode = ProgrammerErrorCode::NoError;
  std::string cmdOutput, outputMsg, listDeviceCmdOutput;
  std::atomic<bool> stopCommand{false};
  devices.clear();
  outputMsg.clear();
  foundTap.clear();

  returnCode = isCableSupported(cable);
  if (returnCode != ProgrammerErrorCode::NoError) {
    return returnCode;
  }

  std::string scanChainCmd = libOpenOcdExecPath + buildScanChainCommand(cable);
  // debug code
  // CFG_POST_MSG("scanChainCmd: %s", scanChainCmd.c_str());
  returnCode = CFG_execute_cmd(scanChainCmd, cmdOutput, nullptr, stopCommand);
  if (returnCode) {
    outputMsg = "Failed to execute following command " + scanChainCmd +
                ". Error code: " + std::to_string(returnCode) + "\n";
    outputMsg += "ListDevices() failed.\n";
    addOrUpdateErrorMessage(ProgrammerErrorCode::FailedExecuteCommand,
                            outputMsg);
    returnCode = ProgrammerErrorCode::FailedExecuteCommand;
    return ProgrammerErrorCode::FailedExecuteCommand;
  }
  // parse the output
  auto availableTap = extractTapInfoList(cmdOutput);
  for (auto& tap : availableTap) {
    for (auto& supportedTap : supportedTAP) {
      if (tap.idCode == supportedTap.idCode) {
        foundTap.push_back(TapInfo{tap.index, tap.tapName, tap.enabled,
                                   tap.idCode, tap.expected, tap.irLen,
                                   tap.irCap, tap.irMask});
      }
    }
  }

  if (foundTap.size() == 0) {
    outputMsg = GetErrorMessage(ProgrammerErrorCode::NoSupportedTapFound);
    return ProgrammerErrorCode::NoSupportedTapFound;
  }

  std::string listDeviceCmd =
      libOpenOcdExecPath + buildListDeviceCommand(cable, foundTap);
  returnCode =
      CFG_execute_cmd(listDeviceCmd, listDeviceCmdOutput, nullptr, stopCommand);
  if (returnCode) {
    outputMsg += "Failed to execute following command " + listDeviceCmd +
                 ". Error code: " + std::to_string(returnCode) + "\n";
    outputMsg += "ListDevices() failed.\n";
    addOrUpdateErrorMessage(ProgrammerErrorCode::FailedExecuteCommand,
                            outputMsg);
    returnCode = ProgrammerErrorCode::FailedExecuteCommand;
    return ProgrammerErrorCode::FailedExecuteCommand;
  }
  devices = extractDeviceList(listDeviceCmdOutput);
  return ProgrammerErrorCode::NoError;
}

int GetFpgaStatus(const Cable& cable, const Device& device, CfgStatus& status) {
  CFG_ASSERT_MSG(!libOpenOcdExecPath.empty(),
                 "libOpenOcdExecPath cannot be empty");
  int returnCode = ProgrammerErrorCode::NoError;
  std::string cmdOutput, outputMsg;
  bool found = false;
  std::atomic<bool> stopCommand{false};

  returnCode = isCableSupported(cable);
  if (returnCode != ProgrammerErrorCode::NoError) {
    return returnCode;
  }

  std::string queryFpgaStatusCmd =
      libOpenOcdExecPath + buildFpgaQueryStatusCommand(cable, device);
  returnCode =
      CFG_execute_cmd(queryFpgaStatusCmd, cmdOutput, nullptr, stopCommand);
  if (returnCode) {
    outputMsg = "Failed to execute following command " + queryFpgaStatusCmd +
                ". Error code: " + std::to_string(returnCode) + "\n";
    outputMsg += "GetFpgaStatus() failed.\n";
    addOrUpdateErrorMessage(ProgrammerErrorCode::FailedExecuteCommand,
                            outputMsg);
    return ProgrammerErrorCode::FailedExecuteCommand;
  }
  status = extractStatus(cmdOutput, found);
  if (!found) {
    outputMsg =
        "Failed to extract status from command output:\n" + cmdOutput + "\n";
    outputMsg += "GetFpgaStatus() failed.\n";
    addOrUpdateErrorMessage(ProgrammerErrorCode::FailedToParseOutput,
                            outputMsg);
    returnCode = ProgrammerErrorCode::FailedToParseOutput;
  }
  outputMsg = "FPGA configuration status found.\n";
  return returnCode;
}

int ProgramFpga(const Cable& cable, const Device& device,
                const std::string& bitfile, std::atomic<bool>& stop,
                std::ostream* outStream /*=nullptr*/,
                OutputMessageCallback callbackMsg /*=nullptr*/,
                ProgressCallback callbackProgress /*=nullptr*/) {
  CFG_ASSERT_MSG(!libOpenOcdExecPath.empty(),
                 "libOpenOcdExecPath cannot be empty");
  int returnCode = ProgrammerErrorCode::NoError;
  std::error_code ec;
  std::string errorMessage;
  if (!std::filesystem::exists(bitfile, ec)) {
    errorMessage = "Cannot find bitfile: " + bitfile + ". " + ec.message();
    returnCode = ProgrammerErrorCode::BitfileNotFound;
    addOrUpdateErrorMessage(returnCode, errorMessage);
    return returnCode;
  }
  std::string cmdOutput;
  std::string programFpgaCommand =
      libOpenOcdExecPath + buildFpgaProgramCommand(cable, device, bitfile);
  std::string progressPercentagePattern = "\\d{1,3}\\.\\d{2}%";
  std::regex regexPattern(progressPercentagePattern);
  returnCode = CFG_execute_cmd_with_callback(programFpgaCommand, cmdOutput,
                                             outStream, regexPattern, stop,
                                             callbackProgress, callbackMsg);
  // std::cout << cmdOutput << std::endl;
  if (returnCode) {
    errorMessage = "Failed to execute following command " + programFpgaCommand +
                   ". Error code: " + std::to_string(returnCode) + "\n";
    errorMessage += "ProgramFpga() failed.\n";
    addOrUpdateErrorMessage(ProgrammerErrorCode::FailedExecuteCommand,
                            errorMessage);
    return ProgrammerErrorCode::FailedExecuteCommand;
  }

  // when programming done, openocd will print out "loaded file"
  // loaded file /home/user1/abc.bin to device 0 in 5s 90381us
  size_t pos = cmdOutput.find("loaded file");
  if (pos == std::string::npos) {
    returnCode = ProgrammerErrorCode::FailedToProgramFPGA;
  }
  return returnCode;
}

int ProgramFlash(
    const Cable& cable, const Device& device, const std::string& bitfile,
    std::atomic<bool>& stop,
    ProgramFlashOperation modes /*= (ProgramFlashOperation::Erase |
                                   ProgramFlashOperation::Program)*/
    ,
    std::ostream* outStream /*=nullptr*/,
    OutputMessageCallback callbackMsg /*=nullptr*/,
    ProgressCallback callbackProgress /*=nullptr*/) {
  CFG_ASSERT_MSG(!libOpenOcdExecPath.empty(),
                 "libOpenOcdExecPath cannot be empty");
  int returnCode = ProgrammerErrorCode::NoError;
  std::error_code ec;
  std::string errorMessage;
  if (!std::filesystem::exists(bitfile, ec)) {
    errorMessage = "Cannot find bitfile: " + bitfile + ". " + ec.message();
    returnCode = ProgrammerErrorCode::BitfileNotFound;
    addOrUpdateErrorMessage(returnCode, errorMessage);
    return returnCode;
  }
  std::string cmdOutput;
  std::string programFlashCommand =
      libOpenOcdExecPath +
      buildFlashProgramCommand(cable, device, bitfile, modes);
  std::string progressPercentagePattern = "\\d{1,3}\\.\\d{2}%";
  std::regex regexPattern(progressPercentagePattern);
  returnCode = CFG_execute_cmd_with_callback(programFlashCommand, cmdOutput,
                                             outStream, regexPattern, stop,
                                             callbackProgress, callbackMsg);
  if (returnCode) {
    errorMessage = "Failed to execute following command " +
                   programFlashCommand +
                   ". Error code: " + std::to_string(returnCode) + "\n";
    errorMessage += "ProgramFlash() failed.\n";
    addOrUpdateErrorMessage(ProgrammerErrorCode::FailedExecuteCommand,
                            errorMessage);
    return ProgrammerErrorCode::FailedExecuteCommand;
  }

  // when programming done, openocd will print out "loaded file"
  // loaded file /home/user1/abc.bin to device 0 in 5s 90381us
  size_t pos = cmdOutput.find("loaded file");
  if (pos == std::string::npos) {
    returnCode = ProgrammerErrorCode::FailedToProgramFPGA;
  }
  return 0;
}

}  // namespace FOEDAG
