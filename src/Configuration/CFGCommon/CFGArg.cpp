#include "CFGArg.h"

#include "CFGCommon.h"

CFGArg::CFGArg(const std::string& n, int i, int a, std::vector<CFGArg_RULE> r,
               const char* h)
    : m_name(n), m_min_arg(i), m_max_arg(a), m_rules(r), m_help_msg(h) {
  CFG_ASSERT(m_name.size());
  CFG_ASSERT(m_min_arg >= 0);
  CFG_ASSERT(m_max_arg == -1 || m_max_arg >= m_min_arg);
  CFG_ASSERT(m_help_msg != nullptr);
}

bool CFGArg::parse(int argc, const char** argv,
                   std::vector<std::string>* errors) {
  bool status = true;
  CFGArg_RULE* ptr = nullptr;
  m_help = false;
  for (int i = 0; i < argc; i++) {
    CFG_ASSERT(argv != nullptr);
    CFG_ASSERT(argv[i] != nullptr);
    std::string str = std::string(argv[i]);
    if (str.size()) {
      if (ptr != nullptr) {
        // Take whatever it is
        assign(ptr, str, errors);
        ptr = nullptr;
      } else {
        if (str == "-h" || str == "--help") {
          m_help = true;
          CFG_POST_MSG(m_help_msg);
        } else if (str[0] == '-') {
          if (str.size() >= 2) {
            if (str[1] == '-') {
              // long argument
              if (str.size() >= 4) {
                status = parse_long_option(str, &ptr, errors) & status;
              } else {
                post_error(CFG_print("Argument at index %d (%s) is invalid", i,
                                     str.c_str()),
                           errors);
                status = false;
              }
            } else {
              // short argument
              status = parse_short_option(str, &ptr, errors) & status;
            }
          } else {
            post_error(CFG_print("Argument at index %d (%s) is invalid", i,
                                 str.c_str()),
                       errors);
            status = false;
          }
        } else {
          m_args.push_back(str);
        }
      }
    } else {
      post_error(CFG_print("Argument at index %d is empty", i), errors);
      status = false;
    }
    if (!status && errors == nullptr) {
      // In reality, once detect error, break
      break;
    }
  }
  if (status || errors != nullptr) {
    // It shouldn't be any leftover
    if (ptr != nullptr) {
      post_error(
          CFG_print("Not enough input to assign option %s", ptr->name.c_str()),
          errors);
      status = false;
    }
  }
  if (!m_help || m_count > 0 || errors != nullptr) {
    if (status || errors != nullptr) {
      for (auto& r : m_rules) {
        if (!r.optional && r.count == 0) {
          post_error(CFG_print("Option %s is required", r.name.c_str()),
                     errors);
          status = false;
          if (errors == nullptr) {
            break;
          }
        }
      }
    }
    if (status || errors != nullptr) {
      if (m_min_arg > int(m_args.size())) {
        post_error(
            CFG_print(
                "Minimum of %d argument(s) need to be specified, but found "
                "%d argument(s) is specified",
                m_min_arg, int(m_args.size())),
            errors);
        status = false;
      }
    }
    if (status || errors != nullptr) {
      if (m_max_arg != -1 && m_max_arg < int(m_args.size())) {
        post_error(
            CFG_print(
                "Can only specify maximum of %d argument(s), but found %d "
                "argument(s) is specified",
                m_max_arg, int(m_args.size())),
            errors);
        status = false;
      }
    }
  }
  return status;
}

void CFGArg::post_error(std::string message, std::vector<std::string>* errors) {
  CFG_POST_ERR(message.c_str());
  if (errors != nullptr) {
    errors->push_back(message);
  }
}

bool CFGArg::parse_long_option(const std::string& option, CFGArg_RULE** ptr,
                               std::vector<std::string>* errors) {
  bool status = true;
  CFGArg_RULE* rule = nullptr;
  std::string value = "";
  (*ptr) = nullptr;
  for (auto& r : m_rules) {
    std::string keyword = CFG_print("--%s", r.name.c_str());
    if (keyword.size() <= option.size()) {
      if (option.find(keyword) == 0) {
        value = option.substr(keyword.size());
        rule = const_cast<CFGArg_RULE*>(&r);
        break;
      }
    }
  }
  if (rule == nullptr && option.size() > 7 && option.find("--help=") == 0) {
    status = print_option(option.substr(7), errors);
  } else if (rule != nullptr) {
    status = check_option_value(rule, value, false, ptr, errors);
  } else {
    post_error(CFG_print("Could not match option %s", option.c_str()), errors);
    status = false;
  }
  return status;
}

bool CFGArg::parse_short_option(const std::string& option, CFGArg_RULE** ptr,
                                std::vector<std::string>* errors) {
  bool status = true;
  CFGArg_RULE* rule = nullptr;
  std::string value = "";
  (*ptr) = nullptr;
  for (auto& r : m_rules) {
    if (r.short_name != char(0)) {
      std::string keyword = CFG_print("-%c", r.short_name);
      if (keyword.size() <= option.size()) {
        if (option.find(keyword) == 0) {
          value = option.substr(keyword.size());
          rule = const_cast<CFGArg_RULE*>(&r);
          break;
        }
      }
    }
  }
  if (rule != nullptr) {
    status = check_option_value(rule, value, true, ptr, errors);
  } else {
    post_error(CFG_print("Could not match option %s", option.c_str()), errors);
    status = false;
  }
  return status;
}

bool CFGArg::check_option_value(CFGArg_RULE* rule, const std::string& value,
                                bool is_short, CFGArg_RULE** ptr,
                                std::vector<std::string>* errors) {
  bool status = true;
  (*ptr) = nullptr;
  if (rule->type == "flag") {
    if (rule->count == 0) {
      if (value.size()) {
        post_error(
            CFG_print("Option %s is a flag, it should not have value (%s)",
                      rule->name.c_str(), value.c_str()),
            errors);
        status = false;
      } else {
        // toggle
        bool* v_ptr = reinterpret_cast<bool*>(const_cast<void*>(rule->ptr));
        (*v_ptr) = !(*v_ptr);
        rule->count++;
        m_count++;
      }
    } else {
      post_error(
          CFG_print(
              "Option %s is a flag, it cannot be specified more than once",
              rule->name.c_str()),
          errors);
      status = false;
    }
  } else {
    if (value.size()) {
      if (is_short) {
        status = assign(rule, value, errors);
      } else {
        if (value[0] == '=') {
          status = assign(rule, value.substr(1), errors);
        } else {
          post_error(
              CFG_print("Long option's value should be assigned using '=' "
                        "sign, example "
                        "--option=value, but found invalid syntax --%s%s",
                        rule->name.c_str(), value.c_str()),
              errors);
          status = false;
        }
      }
    } else {
      // Next argument must be value then
      (*ptr) = rule;
    }
  }
  return status;
}

bool CFGArg::assign(CFGArg_RULE* rule, const std::string& value,
                    std::vector<std::string>* errors) {
  bool status = true;
  if (!rule->multiple && rule->count != 0) {
    post_error(CFG_print("Option %s does not support multiple assignment",
                         rule->name.c_str()),
               errors);
    status = false;
  } else {
    if (rule->type == "int") {
      if (value.size()) {
        uint64_t v = CFG_convert_string_to_u64(value, true, &status);
        if (status) {
          if (rule->multiple) {
            std::vector<uint64_t>* v_ptr =
                reinterpret_cast<std::vector<uint64_t>*>(
                    const_cast<void*>(rule->ptr));
            if (rule->count == 0) {
              // clear whatever default if there is
              v_ptr->clear();
            }
            v_ptr->push_back(v);
          } else {
            uint64_t* v_ptr =
                reinterpret_cast<uint64_t*>(const_cast<void*>(rule->ptr));
            (*v_ptr) = v;
          }
          rule->count++;
          m_count++;
        } else {
          post_error(
              CFG_print(
                  "Fail to assign value %s to option %s because of invalid "
                  "integer conversion",
                  value.c_str(), rule->name.c_str()),
              errors);
        }
      } else {
        post_error(CFG_print("Option %s is an integer, value cannot be empty",
                             rule->name.c_str()),
                   errors);
        status = false;
      }
    } else if (rule->type == "str") {
      // str can accept empty
      if (rule->multiple) {
        std::vector<std::string>* v_ptr =
            reinterpret_cast<std::vector<std::string>*>(
                const_cast<void*>(rule->ptr));
        if (rule->count == 0) {
          // clear whatever default if there is
          v_ptr->clear();
        }
        v_ptr->push_back(value);
      } else {
        std::string* v_ptr =
            reinterpret_cast<std::string*>(const_cast<void*>(rule->ptr));
        (*v_ptr) = value;
      }
      rule->count++;
      m_count++;
    } else {
      CFG_ASSERT(rule->type == "enum");
      CFG_ASSERT(!rule->multiple);
      if (value.size()) {
        int index = CFG_find_string_in_vector(rule->enums, value);
        if (index != -1) {
          std::string* v_ptr =
              reinterpret_cast<std::string*>(const_cast<void*>(rule->ptr));
          (*v_ptr) = value;
          rule->count++;
          m_count++;
        } else {
          post_error(
              CFG_print("Option %s value %s does not match any supported enum",
                        rule->name.c_str(), value.c_str()),
              errors);
          status = false;
        }
      } else {
        post_error(CFG_print("Option %s is an enum, value cannot be empty",
                             rule->name.c_str()),
                   errors);
        status = false;
      }
    }
  }
  return status;
}

bool CFGArg::print_option(const std::string& option,
                          std::vector<std::string>* errors) {
  bool status = false;
  for (auto& r : m_rules) {
    if (option == r.name || (r.short_name != char(0) && option.size() == 1 &&
                             r.short_name == option[0])) {
      std::string message = CFG_print("\n\n%s\n", r.name.c_str());
      if (r.short_name != char(0)) {
        message =
            CFG_print("%s  Short Name : %c\n", message.c_str(), r.short_name);
      }
      message = CFG_print("%s  Type       : %s\n", message.c_str(),
                          r.type_name.c_str());
      message = CFG_print("%s  Optional   : %s\n", message.c_str(),
                          r.optional ? "true" : "false");
      message = CFG_print("%s  Assignment : %s\n", message.c_str(),
                          r.multiple ? "multiple" : "single");
      message = CFG_print("%s  Usage      : %s\n", message.c_str(),
                          r.help[0].c_str());
      if (r.help.size() > 1) {
        for (size_t i = 1; i < r.help.size(); i++) {
          message = CFG_print("%s               %s\n", message.c_str(),
                              r.help[i].c_str());
        }
      }
      CFG_POST_MSG(message.c_str());
      status = true;
      m_help = true;
      break;
    }
  }
  if (!status) {
    post_error(CFG_print("Not able to print help for invalid option %s",
                         option.c_str()),
               errors);
  }
  return status;
}

void CFGArg::print() {
  std::string message = CFG_print("\n\nARG: %s\n", m_name.c_str());
  if (m_rules.size()) {
    message = CFG_print("%s  Option(s)\n", message.c_str());
    for (auto& r : m_rules) {
      message = CFG_print("%s    %s\n", message.c_str(), r.name.c_str());
      message =
          CFG_print("%s      Type: %s\n", message.c_str(), r.type_name.c_str());
      message = CFG_print("%s      Specified: %d\n", message.c_str(), r.count);
      message = CFG_print("%s      Value:", message.c_str());
      if (r.multiple) {
        if (r.type == "str") {
          std::vector<std::string>* v_ptr =
              reinterpret_cast<std::vector<std::string>*>(
                  const_cast<void*>(r.ptr));
          for (auto string : *v_ptr) {
            message = CFG_print("%s %s,", message.c_str(), string.c_str());
          }
        } else {
          std::vector<uint64_t>* v_ptr =
              reinterpret_cast<std::vector<uint64_t>*>(
                  const_cast<void*>(r.ptr));
          for (auto u64 : *v_ptr) {
            message = CFG_print("%s %ld (0x%lx),", message.c_str(), u64, u64);
          }
        }
        message.pop_back();
        message = CFG_print("%s\n", message.c_str());
      } else {
        if (r.type == "str" || r.type == "enum") {
          std::string* v_ptr =
              reinterpret_cast<std::string*>(const_cast<void*>(r.ptr));
          message = CFG_print("%s %s\n", message.c_str(), v_ptr->c_str());
        } else if (r.type == "flag") {
          bool* v_ptr = reinterpret_cast<bool*>(const_cast<void*>(r.ptr));
          message = CFG_print("%s %d\n", message.c_str(), *v_ptr);
        } else {
          uint64_t* v_ptr =
              reinterpret_cast<uint64_t*>(const_cast<void*>(r.ptr));
          message =
              CFG_print("%s %ld (0x%lx)\n", message.c_str(), *v_ptr, *v_ptr);
        }
      }
    }
  }
  if (m_args.size()) {
    message = CFG_print("%s  Argument(s)\n", message.c_str());
    for (size_t i = 0; i < m_args.size(); i++) {
      message =
          CFG_print("%s    (%d) %s\n", message.c_str(), i, m_args[i].c_str());
    }
  }
  CFG_POST_MSG(message.c_str());
}

#include "CFGArg_auto.cpp"
