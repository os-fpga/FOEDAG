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

#include <numeric>  // for std::accumulate
#include <sstream>  // for std::stringstream
#include <thread>   // for std::this_thread::sleep_for

#include "CFGCommon/CFGArg_auto.h"
#include "CFGCommon/CFGCommon.h"
#include "Configuration/HardwareManager/HardwareManager.h"
#include "Configuration/HardwareManager/OpenocdAdapter.h"
#include "ProgrammerGuiInterface.h"
#include "ProgrammerTool.h"
#include "Programmer_error_code.h"
#include "Programmer_helper.h"
#include "tcl.h"
namespace FOEDAG {

// openOCDPath used by library
static std::string libOpenOcdExecPath;

static std::map<Cable, uint32_t, CompareCable> cableSpeedMap = {};

uint32_t GetCableSpeedFromMap(const Cable& cable) {
  auto cableIter = cableSpeedMap.find(cable);
  if (cableIter != cableSpeedMap.end()) {
    return cableIter->second;
  }
  return HM_DEFAULT_CABLE_SPEED_KHZ;
}

std::map<int, std::string> ErrorMessages = {
    {NoError, "Success"},
    {GeneralCmdError, "Command execution error"},
    {UnknownFirmware, "Unknown firmware"},
    {BufferTimeout, "Buffer time out"},
    {CmdTimeout, "Command time out"},
    {ConfigError, "Configuration error"},
    {FsblBootFail, "FSBL boot failed"},
    {CableNotFound, "Cable not found"},
    {CableNotSupported, "Cable not supported"},
    {DeviceNotFound, "FPGA Device not found"},
    {BitfileNotFound, "Bitstream file not found"},
    {OpenOCDExecutableNotFound, "Openocd executable not found"},
    {InvalidFlashSize, "Invalid flash size"}};

void programmer_entry(CFGCommon_ARG* cmdarg) {
  auto arg = std::static_pointer_cast<CFGArg_PROGRAMMER>(cmdarg->arg);
  if (arg == nullptr) return;

  if (arg->m_help) {
    return;
  }
  // setup hardware manager and its depencencies
  OpenocdAdapter openOcd{cmdarg->toolPath.string()};
  HardwareManager hardware_manager{&openOcd};

  std::string subCmd = arg->get_sub_arg_name();
  if (cmdarg->compilerName == "dummy") {
    Cable cable1{};
    cable1.index = 1;
    cable1.name = "UsbProgrammerCable_1_1";
    Cable cable2{};
    cable2.name = "UsbProgrammerCable_1_2";
    cable2.index = 2;
    Device device1{};
    Device device2{};
    device1.name = device2.name = "Gemini";
    device1.index = 1;
    device2.index = 2;
    device1.flashSize = device2.flashSize = 16384;

    if (subCmd == "list_device") {
      auto list_device =
          static_cast<const CFGArg_PROGRAMMER_LIST_DEVICE*>(arg->get_sub_arg());
      processDeviceList(cable1, {device1, device2}, list_device->verbose);
      cmdarg->tclOutput =
          "UsbProgrammerCable_1_1-Gemini<1>-16KB "
          "UsbProgrammerCable_1_1-Gemini<2>-16KB";
    } else if (subCmd == "list_cable") {
      auto list_cable_arg =
          static_cast<const CFGArg_PROGRAMMER_LIST_CABLE*>(arg->get_sub_arg());
      processCableList({cable1, cable2}, list_cable_arg->verbose);
      cmdarg->tclOutput = "UsbProgrammerCable_1_1 UsbProgrammerCable_1_2";
    } else if (subCmd == "fpga_status") {
      cmdarg->tclOutput = "1 0";
    } else if (subCmd == "fpga_config") {
      auto fpga_config_arg =
          static_cast<const CFGArg_PROGRAMMER_FPGA_CONFIG*>(arg->get_sub_arg());
      std::string bitstreamFile = fpga_config_arg->m_args[0];
      std::string cableInput = fpga_config_arg->cable;
      uint64_t deviceIndex = fpga_config_arg->index;
      auto device = deviceIndex == 1 ? device1 : device2;
      if (Gui::GuiInterface())
        Gui::GuiInterface()->ProgramFpga(cable1, device, bitstreamFile);
      int status{TCL_OK};
      for (int i = 10; i <= 100; i += 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (Gui::GuiInterface())
          Gui::GuiInterface()->Progress(std::to_string(i));
        if (Gui::GuiInterface() && Gui::GuiInterface()->Stop()) {
          status = TCL_ERROR;
          break;
        }
        CFG_POST_MSG("<test> program fpga - %d %%", i);
      }
      if (Gui::GuiInterface())
        Gui::GuiInterface()->Status(cable1, device, status);
    } else if (subCmd == "otp") {
      auto fpga_config_arg =
          static_cast<const CFGArg_PROGRAMMER_OTP*>(arg->get_sub_arg());
      std::string bitstreamFile = fpga_config_arg->m_args[0];
      std::string cableInput = fpga_config_arg->cable;
      uint64_t deviceIndex = fpga_config_arg->index;
      auto device = deviceIndex == 1 ? device1 : device2;
      if (Gui::GuiInterface())
        Gui::GuiInterface()->ProgramOtp(cable1, device, bitstreamFile);
      int status{TCL_OK};
      for (int i = 10; i <= 100; i += 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (Gui::GuiInterface())
          Gui::GuiInterface()->Progress(std::to_string(i));
        if (Gui::GuiInterface() && Gui::GuiInterface()->Stop()) {
          status = TCL_ERROR;
          break;
        }
        CFG_POST_MSG("<test> program otp - %d %%", i);
      }
      if (Gui::GuiInterface())
        Gui::GuiInterface()->Status(cable1, device, status);
    } else if (subCmd == "flash") {
      auto flash_arg =
          static_cast<const CFGArg_PROGRAMMER_FLASH*>(arg->get_sub_arg());
      std::string bitstreamFile = flash_arg->m_args[0];
      std::string cableInput = flash_arg->cable;
      uint64_t deviceIndex = flash_arg->index;
      auto device = deviceIndex == 1 ? device1 : device2;
      if (Gui::GuiInterface())
        Gui::GuiInterface()->Flash(cable1, device, bitstreamFile);
      auto operations = parseOperationString(flash_arg->operations);
      if (isOperationRequested("erase", operations)) {
        CFG_POST_MSG("<test> Erasing flash memory");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          if (Gui::GuiInterface())
            Gui::GuiInterface()->Progress(std::to_string(i));
          CFG_POST_MSG("<test> erase flash - %d %% ", i);
        }
      }
      if (isOperationRequested("blankcheck", operations)) {
        CFG_POST_MSG("<test> Flash blank check start ...");
        CFG_POST_MSG("<test> Flash blank check complete.");
      }
      int status = TCL_OK;
      if (isOperationRequested("program", operations)) {
        CFG_POST_MSG("<test> Programming flash memory");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          if (Gui::GuiInterface())
            Gui::GuiInterface()->Progress(std::to_string(i));
          if (Gui::GuiInterface() && Gui::GuiInterface()->Stop()) {
            status = TCL_ERROR;
            break;
          }
          CFG_POST_MSG("<test> program flash - %d %% ", i);
        }
        if (Gui::GuiInterface())
          Gui::GuiInterface()->Status(cable1, device, status);
      }
      if (isOperationRequested("verify", operations)) {
        CFG_POST_MSG("<test> Flash verification start ...");
        for (int i = 10; i <= 100; i += 10) {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          CFG_POST_MSG("<test> flash verified- %d %% ", i);
        }
      }
    } else if (subCmd == "jtag_frequency") {
      auto jtag_frequency_arg =
          static_cast<const CFGArg_PROGRAMMER_JTAG_FREQUENCY*>(
              arg->get_sub_arg());
      std::string cableInput = jtag_frequency_arg->cable;
      if (jtag_frequency_arg->m_args.size() == 1) {
        cmdarg->tclOutput = "";
      } else {
        cmdarg->tclOutput = "1000";
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
    int status = 0;
    InitLibrary(openOcdExecPath.string());
    if (subCmd == "list_device") {
      auto list_device_arg =
          static_cast<const CFGArg_PROGRAMMER_LIST_DEVICE*>(arg->get_sub_arg());
      std::vector<Device> devices{};

      if (list_device_arg->m_args.size() == 1) {
        std::string cable_name = list_device_arg->m_args[0];
        if (!hardware_manager.is_cable_exists(cable_name, true)) {
          CFG_POST_ERR("Cable '%s' not found", cable_name.c_str());
          return;
        }
        devices = hardware_manager.get_devices(cable_name, true);
        if (!devices.empty()) {
          processDeviceList(devices[0].cable, devices,
                            list_device_arg->verbose);
          cmdarg->tclOutput =
              buildCableDevicesAliasNameWithSpaceSeparatedString(
                  devices[0].cable, devices);
        }
      } else {
        // fild all devices
        auto cables = hardware_manager.get_cables();
        if (cables.empty()) {
          CFG_POST_ERR("No cable is connected.");
          return;
        }
        for (auto& cable : cables) {
          auto devices = hardware_manager.get_devices(cable);
          processDeviceList(cable, devices, list_device_arg->verbose);
          cmdarg->tclOutput +=
              buildCableDevicesAliasNameWithSpaceSeparatedString(cable,
                                                                 devices) +
              " ";
        }
      }
    } else if (subCmd == "list_cable") {
      auto list_cable_arg =
          static_cast<const CFGArg_PROGRAMMER_LIST_CABLE*>(arg->get_sub_arg());
      auto cables = hardware_manager.get_cables();
      processCableList(cables, list_cable_arg->verbose);
      if (!cables.empty()) {
        std::string cableNamesTclOuput =
            std::accumulate(cables.begin(), cables.end(), std::string(),
                            [](const std::string& a, const Cable& c) {
                              return a + (a.empty() ? "" : " ") + c.name;
                            });
        cmdarg->tclOutput = cableNamesTclOuput;
      } else {
        cmdarg->tclOutput.clear();
      }
    } else if (subCmd == "fpga_status") {
      auto fpga_status_arg =
          static_cast<const CFGArg_PROGRAMMER_FPGA_STATUS*>(arg->get_sub_arg());
      std::string cable_name = fpga_status_arg->cable;
      uint64_t device_index = fpga_status_arg->index;
      Device device{};
      std::vector<Tap> taplist{};
      CfgStatus cfgStatus;
      std::string statusPrintOut;
      if (!hardware_manager.is_cable_exists(cable_name, true)) {
        CFG_POST_ERR("Cable '%s' not found", cable_name.c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }

      if (!hardware_manager.find_device(cable_name, device_index, device,
                                        taplist, true)) {
        CFG_POST_ERR("Device %d not found", device_index);
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }

      ProgrammerTool programmer{&openOcd};
      int returnCode =
          programmer.query_fpga_status(device, cfgStatus, statusPrintOut);
      if (returnCode != 0) {
        CFG_POST_ERR(
            "Failed to get available devices status. Error code: %d. %s",
            returnCode, GetErrorMessage(returnCode).c_str());
      } else {
        if (fpga_status_arg->verbose) {
          CFG_POST_MSG("\n%s", statusPrintOut.c_str());
        }
        cmdarg->tclOutput = std::to_string(cfgStatus.cfgDone) + " " +
                            std::to_string(cfgStatus.cfgError);
      }
    } else if (subCmd == "fpga_config") {
      auto fpga_config_arg =
          static_cast<const CFGArg_PROGRAMMER_FPGA_CONFIG*>(arg->get_sub_arg());
      std::string bitstreamFile = fpga_config_arg->m_args[0];
      std::string cableInput = fpga_config_arg->cable;
      uint64_t deviceIndex = fpga_config_arg->index;

      Device device{};
      std::vector<Tap> taplist{};
      std::string statusPrintOut;
      if (!hardware_manager.is_cable_exists(cableInput, true)) {
        CFG_POST_ERR("Cable '%s' not found", cableInput.c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }

      if (!hardware_manager.find_device(cableInput, deviceIndex, device,
                                        taplist, true)) {
        CFG_POST_ERR("Device %d not found", deviceIndex);
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }
      openOcd.update_taplist(taplist);
      ProgrammerTool programmer{&openOcd};
      auto speed = GetCableSpeedFromMap(device.cable);
      device.cable.speed = speed;
      std::atomic<bool> stop = false;
      ProgressCallback progress = nullptr;
      auto gui = Gui::GuiInterface();
      if (gui) {
        progress = [gui](const std::string& progress) {
          gui->Progress(progress);
        };
        gui->ProgramFpga(device.cable, device, bitstreamFile);
      }

      status = programmer.program_fpga(
          /*device.cable,*/
          device, bitstreamFile, gui ? gui->Stop() : stop,
          nullptr, /*out stream*/
          [](std::string msg) {
            CFG_post_msg(CFG_print("Progress....%s%%", msg.c_str()),
                         "INFO: ", false);
          },
          progress);
      if (Gui::GuiInterface()) {
        Gui::GuiInterface()->Status(device.cable, device, status);
      }

      if (status != ProgrammerErrorCode::NoError) {
        CFG_POST_ERR("Failed to program %s FPGA. Error code: %d. %s",
                     bitstreamFile.c_str(), status,
                     GetErrorMessage(status).c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      } else {
        CFG_POST_MSG("Programmed '%s' successfully.", bitstreamFile.c_str());
      }
    } else if (subCmd == "otp") {
      auto otp_arg =
          static_cast<const CFGArg_PROGRAMMER_OTP*>(arg->get_sub_arg());
      if (otp_arg->confirm == false) {
        CFG_post_msg(
            "WARNING: The OTP programming is not reversable. Please use -y to "
            "indicate your consensus to proceed.\n\n",
            "", false);
        return;
      }
      std::string bitstreamFile = otp_arg->m_args[0];
      std::string cableInput = otp_arg->cable;
      uint64_t deviceIndex = otp_arg->index;

      Device device{};
      std::vector<Tap> taplist{};
      std::string statusPrintOut;
      if (!hardware_manager.is_cable_exists(cableInput, true)) {
        CFG_POST_ERR("Cable '%s' not found", cableInput.c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }

      if (!hardware_manager.find_device(cableInput, deviceIndex, device,
                                        taplist, true)) {
        CFG_POST_ERR("Device %d not found", deviceIndex);
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }
      openOcd.update_taplist(taplist);
      ProgrammerTool programmer{&openOcd};
      auto speed = GetCableSpeedFromMap(device.cable);
      device.cable.speed = speed;
      std::atomic<bool> stop = false;
      ProgressCallback progress = nullptr;
      auto gui = Gui::GuiInterface();
      if (gui) {
        progress = [gui](const std::string& progress) {
          gui->Progress(progress);
        };
        gui->ProgramOtp(device.cable, device, bitstreamFile);
      }

      status = programmer.program_otp(
          device, bitstreamFile, gui ? gui->Stop() : stop,
          nullptr, /*out stream*/
          [](std::string msg) {
            CFG_post_msg(CFG_print("Progress....%s%%", msg.c_str()),
                         "INFO: ", false);
          },
          progress);
      if (Gui::GuiInterface()) {
        Gui::GuiInterface()->Status(device.cable, device, status);
      }

      if (status != ProgrammerErrorCode::NoError) {
        CFG_POST_ERR("Failed to program device OTP %s. Error code: %d. %s",
                     bitstreamFile.c_str(), status,
                     GetErrorMessage(status).c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      } else {
        CFG_POST_MSG("Programmed OTP '%s' successfully.",
                     bitstreamFile.c_str());
      }
    } else if (subCmd == "flash") {
      auto flash_arg =
          static_cast<const CFGArg_PROGRAMMER_FLASH*>(arg->get_sub_arg());
      std::string bitstreamFile = flash_arg->m_args[0];
      std::string cableInput = flash_arg->cable;
      uint64_t deviceIndex = flash_arg->index;

      Device device{};
      std::vector<Tap> taplist{};
      std::string statusPrintOut;
      if (!hardware_manager.is_cable_exists(cableInput, true)) {
        CFG_POST_ERR("Cable '%s' not found", cableInput.c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }

      if (!hardware_manager.find_device(cableInput, deviceIndex, device,
                                        taplist, true)) {
        CFG_POST_ERR("Device %d not found", deviceIndex);
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }
      openOcd.update_taplist(taplist);
      ProgrammerTool programmer{&openOcd};
      auto speed = GetCableSpeedFromMap(device.cable);
      device.cable.speed = speed;
      std::atomic<bool> stop = false;
      ProgressCallback progress = nullptr;
      auto gui = Gui::GuiInterface();
      if (gui) {
        progress = [gui](const std::string& progress) {
          gui->Progress(progress);
        };
        gui->Flash(device.cable, device, bitstreamFile);
      }

      status = programmer.program_flash(
          device, bitstreamFile, gui ? gui->Stop() : stop,
          ProgramFlashOperation::Program, nullptr, /*out stream*/
          [](std::string msg) {
            CFG_post_msg(CFG_print("Progress....%s%%", msg.c_str()),
                         "INFO: ", false);
          },
          progress);
      if (Gui::GuiInterface()) {
        Gui::GuiInterface()->Status(device.cable, device, status);
      }

      if (status != ProgrammerErrorCode::NoError) {
        CFG_POST_ERR("Failed Flash programming %s FPGA. Error code: %d. %s",
                     bitstreamFile.c_str(), status,
                     GetErrorMessage(status).c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      } else {
        CFG_POST_MSG("Flash programming '%s' successfully.",
                     bitstreamFile.c_str());
      }
    } else if (subCmd == "jtag_frequency") {
      Cable cable;
      uint32_t speed;
      auto jtag_frequency_arg =
          static_cast<const CFGArg_PROGRAMMER_JTAG_FREQUENCY*>(
              arg->get_sub_arg());
      std::string cableInput = jtag_frequency_arg->cable;
      if (!hardware_manager.is_cable_exists(cableInput, true, cable)) {
        CFG_POST_ERR("Cable '%s' not found", cableInput.c_str());
        cmdarg->tclStatus = TCL_ERROR;
        return;
      }
      if (jtag_frequency_arg->m_args.size() == 1) {
        speed = static_cast<uint32_t>(
            CFG_convert_string_to_u64(jtag_frequency_arg->m_args[0]));
        // set jtag speed
        cableSpeedMap[cable] = speed;
        cmdarg->tclOutput = "";
      } else {
        // return jtag speed
        auto it = cableSpeedMap.find(cable);
        if (it != cableSpeedMap.end()) {
          speed = it->second;
        } else {
          speed = 1000;
        }
        cmdarg->tclOutput = std::to_string(speed);
      }
    }
  }
}

int InitLibrary(std::string openOCDPath) {
  if (openOCDPath.empty()) {
    return ProgrammerErrorCode::OpenOCDExecutableNotFound;
  }

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
  return "Unknown error code";
}

int GetAvailableCables(std::vector<Cable>& cables) {
  OpenocdAdapter openOcd{libOpenOcdExecPath};
  HardwareManager hardware_manager{&openOcd};
  cables.clear();
  cables = hardware_manager.get_cables();
  return ProgrammerErrorCode::NoError;
}

int ListDevices(const Cable& cable, std::vector<Device>& devices) {
  OpenocdAdapter openOcd{libOpenOcdExecPath};
  HardwareManager hardware_manager{&openOcd};
  if (!hardware_manager.is_cable_exists(cable.name)) {
    return ProgrammerErrorCode::CableNotFound;
  }
  devices.clear();
  devices = hardware_manager.get_devices(cable);
  if (devices.empty()) {
    return ProgrammerErrorCode::DeviceNotFound;
  }
  return ProgrammerErrorCode::NoError;
}

int GetFpgaStatus(const Cable& cable, const Device& device,
                  CfgStatus& cfgStatus, std::string& statusOutputPrint) {
  OpenocdAdapter openOcd{libOpenOcdExecPath};
  HardwareManager hardware_manager{&openOcd};
  ProgrammerTool programmer{&openOcd};
  Device detectedDevice;
  std::vector<Tap> taplist{};
  int result = CheckCableAndDevice(hardware_manager, cable, device,
                                   detectedDevice, taplist);
  if (result != ProgrammerErrorCode::NoError) {
    return result;
  }
  return programmer.query_fpga_status(device, cfgStatus, statusOutputPrint);
}

int ProgramFpga(const Cable& cable, const Device& device,
                const std::string& bitfile, std::atomic<bool>& stop,
                std::ostream* outStream /*=nullptr*/,
                OutputMessageCallback callbackMsg /*=nullptr*/,
                ProgressCallback callbackProgress /*=nullptr*/) {
  OpenocdAdapter openOcd{libOpenOcdExecPath};
  HardwareManager hardware_manager{&openOcd};
  Device detectedDevice;
  std::vector<Tap> taplist{};
  std::string statusPrintOut;
  int result = CheckCableAndDevice(hardware_manager, cable, device,
                                   detectedDevice, taplist);
  if (result != ProgrammerErrorCode::NoError) {
    return result;
  }
  openOcd.update_taplist(taplist);
  ProgrammerTool programmer{&openOcd};
  return programmer.program_fpga(device, bitfile, stop, outStream, callbackMsg,
                                 callbackProgress);
}

int ProgramOTP(const Cable& cable, const Device& device,
               const std::string& bitfile, std::atomic<bool>& stop,
               std::ostream* outStream /*=nullptr*/,
               OutputMessageCallback callbackMsg /*=nullptr*/,
               ProgressCallback callbackProgress /*=nullptr*/) {
  OpenocdAdapter openOcd{libOpenOcdExecPath};
  HardwareManager hardware_manager{&openOcd};
  Device detectedDevice;
  std::vector<Tap> taplist{};
  std::string statusPrintOut;
  int result = CheckCableAndDevice(hardware_manager, cable, device,
                                   detectedDevice, taplist);
  if (result != ProgrammerErrorCode::NoError) {
    return result;
  }
  openOcd.update_taplist(taplist);
  ProgrammerTool programmer{&openOcd};
  return programmer.program_otp(device, bitfile, stop, outStream, callbackMsg,
                                callbackProgress);
}

int ProgramFlash(const Cable& cable, const Device& device,
                 const std::string& bitfile, std::atomic<bool>& stop,
                 ProgramFlashOperation modes /*= (Erase |
                                                 Program)*/
                 ,
                 std::ostream* outStream /*=nullptr*/,
                 OutputMessageCallback callbackMsg /*=nullptr*/,
                 ProgressCallback callbackProgress /*=nullptr*/) {
  OpenocdAdapter openOcd{libOpenOcdExecPath};
  HardwareManager hardware_manager{&openOcd};
  Device detectedDevice;
  std::vector<Tap> taplist{};
  std::string statusPrintOut;
  int result = CheckCableAndDevice(hardware_manager, cable, device,
                                   detectedDevice, taplist);
  if (result != ProgrammerErrorCode::NoError) {
    return result;
  }
  openOcd.update_taplist(taplist);
  ProgrammerTool programmer{&openOcd};
  return programmer.program_flash(device, bitfile, stop, modes, outStream,
                                  callbackMsg, callbackProgress);
}

}  // namespace FOEDAG
