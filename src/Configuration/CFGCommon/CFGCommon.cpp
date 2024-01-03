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
#include "CFGCommon.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>

static cfg_callback_post_msg_function m_msg_function = nullptr;
static cfg_callback_post_err_function m_err_function = nullptr;
static cfg_callback_execute_command m_execute_cmd_function = nullptr;

class CFG_Exception : public std::exception {
 public:
  CFG_Exception(const std::string& err) : m_err(err) { std::exception(); }

 private:
  virtual const char* what() const throw() { return m_err.c_str(); }
  const std::string m_err = "";
};

std::string CFG_print(const char* format_string, ...) {
  char* buf = nullptr;
  size_t bufsize = CFG_PRINT_MINIMUM_SIZE;
  std::string string = "";
  va_list args;
  while (1) {
    buf = new char[bufsize + 1]();
    memset(buf, 0, bufsize + 1);
    va_start(args, format_string);
    size_t n = std::vsnprintf(buf, bufsize + 1, format_string, args);
    va_end(args);
    if (n <= bufsize) {
      string.resize(n);
      memcpy((char*)(&string[0]), buf, n);
      break;
    }
    delete[] buf;
    buf = nullptr;
    bufsize *= 2;
    if (bufsize > CFG_PRINT_MAXIMUM_SIZE) {
      break;
    }
  }
  if (buf != nullptr) {
    delete[] buf;
  }
  return string;
}

void CFG_assertion(const char* file, const char* func, size_t line,
                   std::string msg) {
  std::string filepath = CFG_change_directory_to_linux_format(file);
  filepath = CFG_get_configuration_relative_path(filepath);
  std::string err = func != nullptr
                        ? CFG_print("Assertion at %s (func: %s, line: %d)",
                                    filepath.c_str(), func, (uint32_t)(line))
                        : CFG_print("Assertion at %s (line: %d)",
                                    filepath.c_str(), (uint32_t)(line));
  CFG_POST_ERR_NO_APPEND(err.c_str());
#if 0
  if (m_err_function != nullptr) {
    printf("*************************************************\n");
    printf("* %s\n", err.c_str());
  }
#endif
  err = CFG_print("  MSG: %s", msg.c_str());
  CFG_POST_ERR_NO_APPEND(err.c_str());
#if 0
  if (m_err_function != nullptr) {
    printf("* %s\n", err.c_str());
    printf("*************************************************\n");
  }
#endif
  throw CFG_Exception(msg);
}

std::string CFG_get_time() {
  time_t rawtime;
  struct tm* timeinfo;
  char buffer[80];
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%d %b %Y %H:%M:%S", timeinfo);
  std::string str(buffer);
  return str;
}

CFG_TIME CFG_time_begin() { return std::chrono::high_resolution_clock::now(); }

uint64_t CFG_nano_time_elapse(CFG_TIME begin) {
  CFG_TIME end = std::chrono::high_resolution_clock::now();
  std::chrono::nanoseconds elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
  return (uint64_t)(elapsed.count());
}

float CFG_time_elapse(CFG_TIME begin) {
  return float(CFG_nano_time_elapse(begin) * 1e-9);
}

void CFG_set_callback_message_function(cfg_callback_post_msg_function msg,
                                       cfg_callback_post_err_function err,
                                       cfg_callback_execute_command exec) {
  CFG_set_callback_post_msg_function(msg);
  CFG_set_callback_post_err_function(err);
  CFG_set_callback_exec_cmd_function(exec);
}

void CFG_set_callback_post_msg_function(cfg_callback_post_msg_function msg) {
  if (msg != nullptr) m_msg_function = msg;
}

void CFG_set_callback_post_err_function(cfg_callback_post_err_function err) {
  if (err != nullptr) m_err_function = err;
}

void CFG_set_callback_exec_cmd_function(cfg_callback_execute_command exec) {
  if (exec != nullptr) m_execute_cmd_function = exec;
}

void CFG_unset_callback_message_function() {
  CFG_unset_callback_post_msg_function();
  CFG_unset_callback_post_err_function();
  CFG_unset_callback_exec_cmd_function();
}

void CFG_unset_callback_post_msg_function() { m_msg_function = nullptr; }

void CFG_unset_callback_post_err_function() { m_err_function = nullptr; }

void CFG_unset_callback_exec_cmd_function() {
  m_execute_cmd_function = nullptr;
}

void CFG_post_msg(const std::string& message, const std::string pre_msg,
                  const bool new_line) {
  if (m_msg_function != nullptr) {
    m_msg_function(message, pre_msg.size() == 0 && !new_line);
  } else {
    std::string termination = new_line ? "\n" : "";
    printf("%s%s%s", pre_msg.c_str(), message.c_str(), termination.c_str());
    fflush(stdout);
  }
}

void CFG_post_warning(const std::string& message) {
  CFG_post_msg(message, "WARNING: ");
}

void CFG_post_err(const std::string& message, bool append) {
  if (m_err_function != nullptr) {
    m_err_function(message, append);
  } else {
    printf("ERROR: %s\n", message.c_str());
  }
}

std::string CFG_change_directory_to_linux_format(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  size_t index = path.find("//");
  while (index != std::string::npos) {
    path.erase(index, 1);
    index = path.find("//");
  }
  return path;
}

std::string CFG_get_configuration_relative_path(std::string path) {
  path = CFG_change_directory_to_linux_format(path);
  size_t index = path.rfind("/src/Configuration");
  if (index != std::string::npos) {
    path.erase(0, index + 5);
  }
  return path;
}

void CFG_get_rid_trailing_whitespace(std::string& string,
                                     const std::vector<char> whitespaces) {
  CFG_ASSERT(whitespaces.size());
  while (string.size()) {
    auto iter =
        std::find(whitespaces.begin(), whitespaces.end(), string.back());
    if (iter != whitespaces.end()) {
      string.pop_back();
    } else {
      break;
    }
  }
}

void CFG_get_rid_leading_whitespace(std::string& string,
                                    const std::vector<char> whitespaces) {
  CFG_ASSERT(whitespaces.size());
  while (string.size()) {
    auto iter =
        std::find(whitespaces.begin(), whitespaces.end(), string.front());
    if (iter != whitespaces.end()) {
      string.erase(0, 1);
    } else {
      break;
    }
  }
}

void CFG_get_rid_whitespace(std::string& string,
                            const std::vector<char> whitespaces) {
  CFG_get_rid_trailing_whitespace(string, whitespaces);
  CFG_get_rid_leading_whitespace(string, whitespaces);
}

std::string CFG_string_toupper(std::string& string) {
  transform(string.begin(), string.end(), string.begin(), ::toupper);
  return string;
}

std::string CFG_string_tolower(std::string& string) {
  transform(string.begin(), string.end(), string.begin(), ::tolower);
  return string;
}

uint64_t CFG_convert_string_to_u64(std::string string, bool no_empty,
                                   bool* status,
                                   const uint64_t* const init_value) {
  CFG_ASSERT(!no_empty || !string.empty());
  std::string original_string = string;
  uint64_t value = init_value != NULL ? *init_value : 0;
  bool valid = true;
  bool neg = (string.find("-") == 0);
  if (neg) {
    string.erase(0, 1);
  }
  // 0: normal
  // 1: binary with "b?????"
  // 2: hex string with "0x?????"
  // 3: binary HDL with "??'b????"
  // 4: decimal HDL with "??'d????"
  // 5: hex HDL with "??'h?????"
  // 6: shift with "<<"
  // 7: shift with ">>"
  enum STRING_TYPE {
    DECIMAL_TYPE,
    BIN_TYPE,
    HEX_TYPE,
    HDL_BIN_TYPE,
    HDL_DECIMAL_TYPE,
    HDL_HEX_TYPE,
    SHIFT_LEFT_TYPE,
    SHIFT_RIGHT_TYPE
  };
  uint32_t hdl_size = 64;
  size_t right_index = string.rfind(">>");
  size_t left_index = string.rfind("<<");
  size_t index = std::string::npos;
  STRING_TYPE type = DECIMAL_TYPE;
  if (right_index != std::string::npos &&
      (left_index == std::string::npos || right_index > left_index)) {
    type = SHIFT_RIGHT_TYPE;
    index = right_index;
    valid = index > 0;
    valid = valid && string.size() >= 4;
  } else if (left_index != std::string::npos) {
    type = SHIFT_LEFT_TYPE;
    index = left_index;
    valid = index > 0;
    valid = valid && string.size() >= 4;
  } else if ((index = string.find("'h")) != std::string::npos) {
    type = HDL_HEX_TYPE;
  } else if ((index = string.find("'d")) != std::string::npos) {
    type = HDL_DECIMAL_TYPE;
  } else if ((index = string.find("'b")) != std::string::npos) {
    type = HDL_BIN_TYPE;
  } else if (string.find("0x") == 0) {
    type = HEX_TYPE;
    string.erase(0, 2);
    CFG_string_toupper(string);
    valid = string.size() > 0 && string.size() <= 16;
    valid = valid &&
            string.find_first_not_of("0123456789ABCDEF") == std::string::npos;
  } else if (string.find("b") == 0) {
    type = BIN_TYPE;
    string.erase(0, 1);
    valid = string.size() > 0 && string.size() <= 64;
    valid = valid && string.find_first_not_of("01") == std::string::npos;
  } else {
    valid = string.find_first_not_of("0123456789") == std::string::npos;
  }
  // Second layer of validity checking
  if (valid && (type == HDL_BIN_TYPE || type == HDL_DECIMAL_TYPE ||
                type == HDL_HEX_TYPE)) {
    CFG_ASSERT(index != std::string::npos);
    if (index != 0) {
      valid = string.substr(0, index).find_first_not_of("0123456789") ==
              std::string::npos;
      if (valid) {
        hdl_size = CFG_convert_string_to_u64(string.substr(0, index), true,
                                             &valid, nullptr);
        valid = valid && hdl_size > 0 && hdl_size <= 64;
      }
    }
    if (valid) {
      string = string.substr(index + 2);
      if (type == HDL_BIN_TYPE) {
        valid = string.size() > 0 && string.size() <= 64;
        valid = valid && string.find_first_not_of("01") == std::string::npos;
      } else if (type == HDL_DECIMAL_TYPE) {
        valid = string.size() > 0;
        valid = valid &&
                string.find_first_not_of("0123456789") == std::string::npos;
      } else {
        CFG_string_toupper(string);
        valid = string.size() > 0 && string.size() <= 16;
        valid = valid && string.find_first_not_of("0123456789ABCDEF") ==
                             std::string::npos;
      }
    }
  }
  // Conversion
  if (valid) {
    if (type == DECIMAL_TYPE || type == HDL_DECIMAL_TYPE) {
      value = (type == HDL_DECIMAL_TYPE || string.size() > 0)
                  ? std::strtoull(string.c_str(), nullptr, 10)
                  : value;
    } else if (type == BIN_TYPE || type == HDL_BIN_TYPE) {
      value = std::strtoull(string.c_str(), nullptr, 2);
    } else if (type == HEX_TYPE || type == HDL_HEX_TYPE) {
      value = std::strtoull(string.c_str(), nullptr, 16);
    } else {
      CFG_ASSERT(type == SHIFT_LEFT_TYPE || type == SHIFT_RIGHT_TYPE);
      CFG_ASSERT(index != std::string::npos);
      std::string value_string = string.substr(0, index);
      std::string shift_string = string.substr(index + 2);
      CFG_get_rid_whitespace(value_string);
      CFG_get_rid_whitespace(shift_string);
      if (value_string.size() > 0 && shift_string.size() > 0) {
        uint64_t u64 =
            CFG_convert_string_to_u64(value_string, true, &valid, nullptr);
        if (valid) {
          uint64_t shift =
              CFG_convert_string_to_u64(shift_string, true, &valid, nullptr);
          valid = valid && shift < 64;
          if (valid) {
            value = type == SHIFT_LEFT_TYPE ? (u64 << shift) : (u64 >> shift);
          }
        }
      } else {
        valid = false;
      }
    }
  }
  CFG_ASSERT_MSG(valid || status != NULL,
                 "Invalid string \"%s\" to uint64_t conversion",
                 original_string.c_str());
  if (valid &&
      (type == HDL_BIN_TYPE || type == HDL_DECIMAL_TYPE ||
       type == HDL_HEX_TYPE) &&
      hdl_size != 64) {
    value = value & (((uint64_t)(1) << hdl_size) - 1);
  }
  if (valid && neg) {
    value = uint64_t(uint64_t(0) - value);
  }
  if (status != NULL) {
    *status = valid;
  }
  return value;
}

std::string CFG_convert_number_to_unit_string(uint64_t number) {
  std::vector<std::string> units = {"K", "M", "G", "T"};
  std::string unit = "";
  for (size_t i = 0; i < units.size(); i++) {
    if (number >= 1024 && (number % 1024) == 0) {
      number = number / 1024;
      unit = units[i];
    } else {
      break;
    }
  }
  return std::to_string(number) + unit;
}

template <typename T>
int CFG_find_element_in_vector(const std::vector<T>& vector, const T element) {
  auto iter = std::find(vector.begin(), vector.end(), element);
  if (iter != vector.end()) {
    return int(std::distance(vector.begin(), iter));
  }
  return (-1);
}

int CFG_find_string_in_vector(const std::vector<std::string>& vector,
                              const std::string element) {
  return CFG_find_element_in_vector(vector, element);
}

int CFG_find_u32_in_vector(const std::vector<uint32_t>& vector,
                           const uint32_t element) {
  return CFG_find_element_in_vector(vector, element);
}

std::vector<std::string> CFG_split_string(const std::string& str,
                                          const std::string& seperator,
                                          int max_split, bool include_empty) {
  CFG_ASSERT(seperator.size());
  CFG_ASSERT(max_split >= 0);
  std::vector<std::string> vec;
  int split = 0;
  size_t search_index = 0;
  size_t index = str.find(seperator, search_index);
  while (index != std::string::npos) {
    if (index != search_index || include_empty) {
      if (index == search_index) {
        vec.push_back("");
      } else {
        vec.push_back(str.substr(search_index, index - search_index));
        search_index += (index - search_index);
      }
      split++;
      if (max_split != 0 && split == max_split) {
        search_index += seperator.size();
        break;
      }
    }
    search_index += seperator.size();
    index = str.find(seperator, search_index);
  }
  CFG_ASSERT(search_index <= str.size());
  if (search_index == str.size()) {
    if (include_empty) {
      vec.push_back("");
    }
  } else {
    vec.push_back(str.substr(search_index, str.size() - search_index));
  }
  return vec;
}

int CFG_compiler_execute_cmd(const std::string& command,
                             const std::string logFile, bool appendLog) {
  if (m_execute_cmd_function != nullptr) {
    return m_execute_cmd_function(command, logFile, appendLog);
  } else {
    std::string output = "";
    std::atomic<bool> stop = false;
    return CFG_execute_cmd(command, output, nullptr, stop);
  }
}

int CFG_execute_cmd(const std::string& cmd, std::string& output,
                    std::ostream* outStream, std::atomic<bool>& stopCommand) {
#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#define WEXITSTATUS
#else
#define POPEN popen
#define PCLOSE pclose
#endif

  FILE* pipe = POPEN(cmd.c_str(), "r");
  if (pipe == nullptr) {
    return -1;
  }

  // Read the output of the command and store it in the output string.
  char buffer[1024];
  std::string newline;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr && !stopCommand) {
    output += buffer;
    newline = buffer;

    if (outStream) {
      *outStream << newline;
    }

    if (stopCommand) {
      break;
    }
  }

  int status = PCLOSE(pipe);
  int exit_code = WEXITSTATUS(status);
  return exit_code;
}

int CFG_execute_cmd_with_callback(
    const std::string& cmd, std::string& output, std::ostream* outStream,
    std::regex patternToMatch, std::atomic<bool>& stopCommand,
    std::function<void(const std::string&)> progressCallback,
    std::function<void(const std::string&)> generalCallback) {
#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#define WEXITSTATUS
#else
#define POPEN popen
#define PCLOSE pclose
#endif

  FILE* pipe = POPEN(cmd.c_str(), "r");
  if (pipe == nullptr) {
    return -1;
  }

  // Read the output of the command and store it in the output string.
  char buffer[1024];
  std::string newline;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr && !stopCommand) {
    output += buffer;
    newline = buffer;
    if (generalCallback != nullptr) {
      generalCallback(newline);
    }

    if (outStream) {
      *outStream << newline;
    }

    if (stopCommand) {
      break;
    }

    std::smatch matches;
    if (progressCallback &&
        std::regex_search(newline, matches, patternToMatch)) {
      progressCallback(matches.str());
    }
  }

  int status = PCLOSE(pipe);
  int exit_code = WEXITSTATUS(status);
  return exit_code;
}

std::filesystem::path CFG_find_file(const std::filesystem::path& filePath,
                                    const std::filesystem::path& defaultDir) {
  if (std::filesystem::is_regular_file(
          filePath)) {  // check if it is already a valid file path
    return std::filesystem::absolute(filePath);
  } else {
    std::filesystem::path file_abs_path = defaultDir / filePath;
    if (std::filesystem::is_regular_file(
            file_abs_path)) {  // check if the file exists in default directory
      return std::filesystem::absolute(file_abs_path);
    } else {
      return "";
    }
  }
}

void CFG_sleep_ms(uint32_t milisecond) {
#ifdef _WIN32
  Sleep(milisecond);
#else
  usleep(milisecond * 1000);
#endif
}

void CFG_read_text_file(const std::string& filepath,
                        std::vector<std::string>& data,
                        bool trim_trailer_whitespace) {
  std::fstream file;
  file.open(filepath.c_str(), std::ios::in);
  CFG_ASSERT_MSG(file.is_open(), "Fail to open %s", filepath.c_str());
  std::string line = "";
  while (getline(file, line)) {
    if (trim_trailer_whitespace) {
      CFG_get_rid_trailing_whitespace(line);
    }
    data.push_back(line);
  }
  file.close();
}

void CFG_read_binary_file(const std::string& filepath,
                          std::vector<uint8_t>& data) {
  // File size to prepare memory
  std::ifstream file(filepath.c_str(), std::ios::binary | std::ios::ate);
  CFG_ASSERT_MSG(file.is_open(), "Fail to open binary file %s",
                 filepath.c_str());
  size_t filesize = file.tellg();
  file.close();

  // Read the binary
  CFG_ASSERT(filesize > 0);
  if (data.size()) {
    memset(&data[0], 0, data.size());
    data.clear();
  }
  data.resize(filesize);
  file = std::ifstream(filepath.c_str(), std::ios::in | std::ios::binary);
  CFG_ASSERT(file.is_open());
  file.read((char*)(&data[0]), data.size());
  file.close();
}

void CFG_write_binary_file(const std::string& filepath, const uint8_t* data,
                           const size_t data_size) {
  std::ofstream file(filepath.c_str(), std::ios::out | std::ios::binary);
  CFG_ASSERT(file.is_open());
  file.write((char*)(const_cast<uint8_t*>(data)), data_size);
  file.flush();
  file.close();
}

bool CFG_compare_two_text_files(const std::string& filepath1,
                                const std::string& filepath2) {
  std::vector<std::string> data1;
  std::vector<std::string> data2;
  CFG_read_text_file(filepath1, data1, false);
  CFG_read_text_file(filepath2, data2, false);
  bool status = false;
  if (data1.size() == data2.size()) {
    status = true;
    for (size_t i = 0; i < data1.size(); i++) {
      if (data1[i] != data2[i]) {
        status = false;
        break;
      }
    }
  }
  return status;
}

bool CFG_compare_two_binary_files(const std::string& filepath1,
                                  const std::string& filepath2) {
  std::vector<uint8_t> data1;
  std::vector<uint8_t> data2;
  CFG_read_binary_file(filepath1, data1);
  CFG_read_binary_file(filepath2, data2);
  return (data1.size() == data2.size()) &&
         (data1.size() == 0 || memcmp(&data1[0], &data2[0], data1.size()) == 0);
}