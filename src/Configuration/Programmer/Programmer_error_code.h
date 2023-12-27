#pragma once
namespace FOEDAG {

enum ProgrammerErrorCode {
  NoError = 0,
  GeneralCmdError = -1,
  UnknownFirmware = -2,
  BufferTimeout = -3,
  CmdTimeout = -4,
  ConfigError = -5,
  FsblBootFail = -6,
  CableNotFound = -7,
  CableNotSupported = -8,
  DeviceNotFound = -9,
  BitfileNotFound = -10,
  OpenOCDExecutableNotFound = -11,
  InvalidFlashSize = -12,
  ParseFpgaStatusError = -13
};

}
