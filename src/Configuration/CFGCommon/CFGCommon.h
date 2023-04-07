#ifndef CFGCommon_H
#define CFGCommon_H

#include <chrono>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#define __FILENAME__ \
  (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define CFG_PRINT_MINIMUM_SIZE (256)
#define CFG_PRINT_MAXIMUM_SIZE (8192)
typedef std::chrono::high_resolution_clock::time_point CFG_TIME;

typedef void (*cfg_callback_post_msg_function)(const std::string& message,
                                               const bool raw);
typedef void (*cfg_callback_post_err_function)(const std::string& message,
                                               bool append);
typedef int (*cfg_callback_execute_command)(const std::string& command,
                                            const std::string logFile,
                                            bool appendLog);

class CFGArg;
struct CFGCommon_ARG {
  std::string command;
  std::string device;
  std::string projectName;
  std::string projectPath;
  std::string compilerName;
  std::filesystem::path toolPath;    // for any tool path
  std::filesystem::path searchPath;  // for any search path
  bool clean;
  std::shared_ptr<CFGArg> arg;
};

std::string CFG_print(const char* format_string, ...);

void CFG_assertion(const char* file, const char* func, size_t line,
                   std::string msg);

#define CFG_INTERNAL_ERROR(...) \
  { CFG_assertion(__FILE__, __func__, __LINE__, CFG_print(__VA_ARGS__)); }

#if defined(_MSC_VER)

#define CFG_ASSERT(truth)                                \
  if (!(truth)) {                                        \
    CFG_assertion(__FILE__, __func__, __LINE__, #truth); \
  }

#define CFG_ASSERT_MSG(truth, ...)                                       \
  if (!(truth)) {                                                        \
    CFG_assertion(__FILE__, __func__, __LINE__, CFG_print(__VA_ARGS__)); \
  }

#else

#define CFG_ASSERT(truth)                                \
  if (!(__builtin_expect(!!(truth), 0))) {               \
    CFG_assertion(__FILE__, __func__, __LINE__, #truth); \
  }

#define CFG_ASSERT_MSG(truth, ...)                                       \
  if (!(__builtin_expect(!!(truth), 0))) {                               \
    CFG_assertion(__FILE__, __func__, __LINE__, CFG_print(__VA_ARGS__)); \
  }

#endif

std::string CFG_get_time();

CFG_TIME CFG_time_begin();

uint64_t CFG_nano_time_elapse(CFG_TIME begin);

float CFG_time_elapse(CFG_TIME begin);

void set_callback_message_function(cfg_callback_post_msg_function msg,
                                   cfg_callback_post_err_function err,
                                   cfg_callback_execute_command exec);

void CFG_post_msg(const std::string& message,
                  const std::string pre_msg = "INFO: ",
                  const bool new_line = true);

void CFG_post_warning(const std::string& message);

void CFG_post_err(const std::string& message, bool append);

int CFG_execute_and_monitor_system_command(
    const std::string& command, const std::string logFile = std::string{},
    bool appendLog = false);

std::string change_directory_to_linux_format(std::string path);

std::string get_configuration_relative_path(std::string path);

void CFG_get_rid_trailing_whitespace(std::string& string,
                                     const std::vector<char> whitespaces = {
                                         ' ', '\t', '\n', '\r'});

void CFG_get_rid_leading_whitespace(std::string& string,
                                    const std::vector<char> whitespaces = {
                                        ' ', '\t', '\n', '\r'});

void CFG_get_rid_whitespace(std::string& string,
                            const std::vector<char> whitespaces = {' ', '\t',
                                                                   '\n', '\r'});
void CFG_string_toupper(std::string& string);

void CFG_string_tolower(std::string& string);

uint64_t CFG_convert_string_to_u64(std::string string, bool no_empty = false,
                                   bool* status = NULL,
                                   uint64_t* init_value = NULL,
                                   bool support_shift = false);

int CFG_find_string_in_vector(const std::vector<std::string>& vector,
                              const std::string element);

int CFG_find_u32_in_vector(const std::vector<uint32_t>& vector,
                           const uint32_t element);

int CFG_compiler_execute_cmd(const std::string& command,
                             const std::string logFile = std::string{},
                             bool appendLog = false);

int CFG_execute_cmd(const std::string& cmd, std::string& output);

std::filesystem::path CFG_find_file(const std::filesystem::path& filePath,
                                    const std::filesystem::path& defaultDir);

#define CFG_POST_MSG(...) \
  { CFG_post_msg(CFG_print(__VA_ARGS__)); }

#define CFG_POST_WARNING(...) \
  { CFG_post_warning(CFG_print(__VA_ARGS__)); }

#define CFG_POST_ERR(...) \
  { CFG_post_err(CFG_print(__VA_ARGS__), true); }

#define CFG_POST_ERR_NO_APPEND(...) \
  { CFG_post_err(CFG_print(__VA_ARGS__), false); }

// Use the normal message for now
#define CFG_POST_DBG(...) \
  { CFG_post_msg(CFG_print(__VA_ARGS__)); }

#endif
