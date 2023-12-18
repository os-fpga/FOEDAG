// ProgrammingAdapter.h
#ifndef PROGRAMMINGADAPTER_H 
#define PROGRAMMINGADAPTER_H

#include <atomic>
#include <functional>
#include <string>

namespace FOEDAG {

struct Device;
struct CfgStatus;
enum class ProgramFlashOperation : uint32_t;
using ProgressCallback = std::function<void(std::string)>;
using OutputMessageCallback = std::function<void(std::string)>;
class ProgrammingAdapter {
 public:
  virtual ~ProgrammingAdapter() = default;

  virtual int program_fpga(const Device& device, const std::string& bitfile,
                           std::atomic<bool>& stop,
                           std::ostream* outStream = nullptr,
                           OutputMessageCallback callbackMsg = nullptr,
                           ProgressCallback callbackProgress = nullptr) = 0;
  virtual int program_flash(const Device& device, const std::string& bitfile,
                            std::atomic<bool>& stop,
                            ProgramFlashOperation modes,
                            std::ostream* outStream = nullptr,
                            OutputMessageCallback callbackMsg = nullptr,
                            ProgressCallback callbackProgress = nullptr) = 0;
  virtual int program_otp(const Device& device, const std::string& bitfile,
                          std::atomic<bool>& stop,
                          std::ostream* outStream = nullptr,
                          OutputMessageCallback callbackMsg = nullptr,
                          ProgressCallback callbackProgress = nullptr) = 0;
  virtual int query_fpga_status(const Device& device, CfgStatus& cfgStatus,
                                std::string& outputMessage) = 0;
};

}  // namespace FOEDAG

#endif  // PROGRAMMINGADAPTER_H