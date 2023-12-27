// ProgrammerTool.h
#ifndef PROGRAMMERTOOL_H
#define PROGRAMMERTOOL_H

#include "Configuration/HardwareManager/ProgrammingAdapter.h"

namespace FOEDAG {

struct Device;
struct Tap;

enum class ProgramFlashOperation : uint32_t;

class ProgrammerTool {
 public:
  ProgrammerTool(ProgrammingAdapter* adapter);
  ~ProgrammerTool();
  int program_fpga(const Device& device, const std::string& bitfile,
                   std::atomic<bool>& stop, std::ostream* outStream = nullptr,
                   OutputMessageCallback callbackMsg = nullptr,
                   ProgressCallback callbackProgress = nullptr);
  int program_flash(const Device& device, const std::string& bitfile,
                    std::atomic<bool>& stop, ProgramFlashOperation modes,
                    std::ostream* outStream = nullptr,
                    OutputMessageCallback callbackMsg = nullptr,
                    ProgressCallback callbackProgress = nullptr);
  int program_otp(const Device& device, const std::string& bitfile,
                  std::atomic<bool>& stop, std::ostream* outStream = nullptr,
                  OutputMessageCallback callbackMsg = nullptr,
                  ProgressCallback callbackProgress = nullptr);
  int query_fpga_status(const Device& device, CfgStatus& cfgStatus,
                        std::string& outputMessage);

 private:
  ProgrammingAdapter* m_adapter;
};

}  // namespace FOEDAG

#endif  // PROGRAMMERTOOL_H
