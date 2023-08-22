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

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <exception>

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
  std::string filepath = change_directory_to_linux_format(file);
  filepath = get_configuration_relative_path(filepath);
  std::string err = func != nullptr
                        ? CFG_print("Assertion at %s (func: %s, line: %d)",
                                    filepath.c_str(), func, (uint32_t)(line))
                        : CFG_print("Assertion at %s (line: %d)",
                                    filepath.c_str(), (uint32_t)(line));
  CFG_POST_ERR(err.c_str());
  if (m_err_function != nullptr) {
    printf("*************************************************\n");
    printf("* %s\n", err.c_str());
  }
  err = CFG_print("  MSG: %s", msg.c_str());
  CFG_POST_ERR(err.c_str());
  if (m_err_function != nullptr) {
    printf("* %s\n", err.c_str());
    printf("*************************************************\n");
  }
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

void set_callback_message_function(cfg_callback_post_msg_function msg,
                                   cfg_callback_post_err_function err,
                                   cfg_callback_execute_command exec) {
  CFG_ASSERT(msg != nullptr);
  CFG_ASSERT(err != nullptr);
  CFG_ASSERT(exec != nullptr);
  m_msg_function = msg;
  m_err_function = err;
  m_execute_cmd_function = exec;
}

void unset_callback_message_function() {
  m_msg_function = nullptr;
  m_err_function = nullptr;
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

std::string change_directory_to_linux_format(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  size_t index = path.find("//");
  while (index != std::string::npos) {
    path.erase(index, 1);
    index = path.find("//");
  }
  return path;
}

std::string get_configuration_relative_path(std::string path) {
  size_t index = path.find("/src/Configuration");
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
                                   bool* status, uint64_t* init_value,
                                   bool support_shift) {
  CFG_ASSERT(!no_empty || !string.empty());
  std::string original_string = string;
  uint64_t value = init_value != NULL ? *init_value : 0;
  bool valid = true;
  bool neg = (string.find("-") == 0);
  if (neg) {
    string.erase(0, 1);
  }
  uint8_t type = 0;
  if (support_shift && string.find(">>") != std::string::npos) {
    type = 4;
    valid = string.find("<<") == std::string::npos;
  } else if (support_shift && string.find("<<") != std::string::npos) {
    type = 3;
    valid = string.find(">>") == std::string::npos;
  } else if (string.find("0x") == 0) {
    type = 2;
    string.erase(0, 2);
    CFG_string_toupper(string);
    valid = string.size() > 0 && string.size() <= 16;
  } else if (string.find("b") == 0) {
    type = 1;
    string.erase(0, 1);
    valid = string.size() > 0 && string.size() <= 64;
  }
  if (valid) {
    std::string::iterator current_iter = string.begin();
    if (type == 0 || type == 1) {
      // Decimal or binary
      const char limit = (type == 0) ? char('9') : char('1');
      while (current_iter != string.end()) {
        if (!(*current_iter >= '0' && *current_iter <= limit)) {
          valid = false;
          break;
        }
        current_iter++;
      }
    } else if (type == 2) {
      // hexa-decimal
      while (current_iter != string.end()) {
        if (!((*current_iter >= '0' && *current_iter <= '9') ||
              (*current_iter >= 'A' && *current_iter <= 'F'))) {
          valid = false;
          break;
        }
        current_iter++;
      }
    } else {
      CFG_ASSERT(type == 3 || type == 4);
      if (string.size() > 4) {
        size_t index = type == 3 ? string.find("<<") : string.find(">>");
        if (index > 0) {
          std::string value_string = string.substr(0, index);
          std::string shift_string = string.substr(index + 2);
          CFG_get_rid_whitespace(value_string);
          CFG_get_rid_whitespace(shift_string);
          if (value_string.size() > 0 && shift_string.size() > 0) {
            uint64_t u64 = CFG_convert_string_to_u64(value_string, true, &valid,
                                                     NULL, false);
            if (valid) {
              uint64_t shift = CFG_convert_string_to_u64(shift_string, true,
                                                         &valid, NULL, false);
              if (valid) {
                value = type == 3 ? (u64 << shift) : (u64 >> shift);
              }
            }
          } else {
            valid = false;
          }
        } else {
          valid = false;
        }
      } else {
        valid = false;
      }
    }
  }
  CFG_ASSERT_MSG(valid || status != NULL,
                 "Invalid string \"%s\" to uint64_t conversion",
                 original_string.c_str());
  if (valid && !string.empty()) {
    if (type == 0) {
      value = std::strtoull(string.c_str(), NULL, 10);
    } else if (type == 1) {
      value = std::strtoull(string.c_str(), NULL, 2);
    } else if (type == 2) {
      value = std::strtoull(string.c_str(), NULL, 16);
    } else {
      CFG_ASSERT(type == 3 || type == 4);
      // value had been converted
    }
    if (neg) {
      value = uint64_t(uint64_t(0) - value);
    }
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
    if (std::regex_search(newline, matches, patternToMatch)) {
      if (progressCallback != nullptr) {
        std::string strOutput = matches.str();
        strOutput += "\n";
        progressCallback(strOutput);
      }
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
