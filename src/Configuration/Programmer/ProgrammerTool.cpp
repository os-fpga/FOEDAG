
#include "ProgrammerTool.h"

#include "Configuration/CFGCommon/CFGCommon.h"
#include "Programmer_error_code.h"

namespace FOEDAG {

ProgrammerTool::ProgrammerTool(ProgrammingAdapter* adapter)
    : m_adapter(adapter) {
  CFG_ASSERT(m_adapter != nullptr);
}

ProgrammerTool::~ProgrammerTool() {}

int ProgrammerTool::program_fpga(
    const Device& device, const std::string& bitfile, std::atomic<bool>& stop,
    std::ostream* outStream /*= nullptr*/,
    OutputMessageCallback callbackMsg /*= nullptr*/,
    ProgressCallback callbackProgress /*= nullptr*/) {
  int statusCode = ProgrammerErrorCode::NoError;
  std::error_code ec;
  std::string errorMessage;
  if (!std::filesystem::exists(bitfile, ec)) {
    return ProgrammerErrorCode::BitfileNotFound;
  }
  statusCode = m_adapter->program_fpga(device, bitfile, stop, outStream,
                                       callbackMsg, callbackProgress);
  return statusCode;
}

int ProgrammerTool::program_flash(
    const Device& device, const std::string& bitfile, std::atomic<bool>& stop,
    ProgramFlashOperation modes, std::ostream* outStream /*= nullptr*/,
    OutputMessageCallback callbackMsg /*= nullptr*/,
    ProgressCallback callbackProgress /*= nullptr*/) {
  int statusCode = ProgrammerErrorCode::NoError;
  std::error_code ec;
  std::string errorMessage;
  if (!std::filesystem::exists(bitfile, ec)) {
    return ProgrammerErrorCode::BitfileNotFound;
  }
  statusCode = m_adapter->program_flash(device, bitfile, stop, modes, outStream,
                                        callbackMsg, callbackProgress);
  return statusCode;
}

int ProgrammerTool::program_otp(
    const Device& device, const std::string& bitfile, std::atomic<bool>& stop,
    std::ostream* outStream /*= nullptr*/,
    OutputMessageCallback callbackMsg /*= nullptr*/,
    ProgressCallback callbackProgress /*= nullptr*/) {
  int statusCode = ProgrammerErrorCode::NoError;
  std::error_code ec;
  std::string errorMessage;
  if (!std::filesystem::exists(bitfile, ec)) {
    return ProgrammerErrorCode::BitfileNotFound;
  }
  statusCode = m_adapter->program_otp(device, bitfile, stop, outStream,
                                      callbackMsg, callbackProgress);
  return statusCode;
}

int ProgrammerTool::query_fpga_status(const Device& device,
                                      CfgStatus& cfgStatus,
                                      std::string& outputMessage) {
  int statusCode = ProgrammerErrorCode::NoError;
  statusCode = m_adapter->query_fpga_status(device, cfgStatus, outputMessage);
  return statusCode;
}

}  // namespace FOEDAG