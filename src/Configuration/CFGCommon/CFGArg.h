#ifndef CFG_ARG_H
#define CFG_ARG_H

#include <map>
#include <string>
#include <vector>

#include "CFGCommon.h"

struct CFGArg_RULE {
  CFGArg_RULE(const std::string& n, char s, const std::string& t, bool o,
              bool m, bool hi, const void* p, const std::vector<std::string> h,
              const std::vector<std::string> e)
      : name(n),
        short_name(s),
        type(t),
        type_name(t == "int" ? "integer" : (t == "str" ? "string" : t)),
        optional(o),
        multiple(m),
        hide(hi),
        ptr(p),
        help(h),
        enums(e) {
    CFG_ASSERT(name.size());
    CFG_ASSERT(type == "flag" || type == "int" || type == "str" ||
               type == "enum");
    CFG_ASSERT(ptr != nullptr);
    if (type == "flag" || type == "enum") {
      CFG_ASSERT(!multiple);
    }
    CFG_ASSERT(help.size());
    for (auto& hl : help) {
      CFG_ASSERT(hl.size());
    }
    if (type == "enum") {
      CFG_ASSERT(enums.size());
    } else {
      CFG_ASSERT(enums.size() == 0);
    }
  }
  const std::string name;
  const char short_name;
  const std::string type;
  const std::string type_name;
  const bool optional;
  const bool multiple;
  const bool hide;
  const void* ptr;
  const std::vector<std::string> help;
  const std::vector<std::string> enums;
  uint32_t count = 0;
};

class CFGArg {
 public:
  static void parse(const std::string& main_command, size_t argc,
                    const std::string* argv,
                    std::vector<std::string>& flag_options,
                    std::map<std::string, std::string>& options,
                    std::vector<std::string>& positional_options,
                    const std::vector<std::string>& supported_flags,
                    const std::vector<std::string>& required_options,
                    const std::vector<std::string>& optional_options,
                    int supported_positional_option_count);
  CFGArg(const std::string& n, bool hidden, int i, int a,
         std::vector<CFGArg_RULE> r, const char* h);
  CFGArg(const std::string& n, bool hidden, const char* h, const char* hh);
  std::string get_sub_arg_name();
  const CFGArg* get_sub_arg();
  bool specified(const std::string& option);
  const std::string m_name = "";
  const bool m_hidden = false;
  std::vector<std::string> m_args;
  bool m_help = false;

 protected:
  bool parse(int argc, const char* argv[], std::vector<std::string>* errors);
  void print() const;

 private:
  bool single_parse(int argc, const char* argv[],
                    std::vector<std::string>* errors);
  void post_error(std::string message, std::vector<std::string>* errors);
  bool parse_long_option(const std::string& option, CFGArg_RULE** ptr,
                         std::vector<std::string>* errors);
  bool parse_short_option(const std::string& option, CFGArg_RULE** ptr,
                          std::vector<std::string>* errors);
  bool check_option_value(CFGArg_RULE* rule, const std::string& value,
                          bool is_short, CFGArg_RULE** ptr,
                          std::vector<std::string>* errors);
  bool assign(CFGArg_RULE* rule, const std::string& value,
              std::vector<std::string>* errors);
  bool print_option(const std::string& option,
                    std::vector<std::string>* errors);
  const int m_min_arg = 0;
  const int m_max_arg = 0;
  const std::vector<CFGArg_RULE> m_rules = {};
  const char* m_help_msg = nullptr;
  const char* m_hidden_help_msg = nullptr;
  uint32_t m_count = 0;
  std::string m_subcmd = "";

 protected:
  const std::map<std::string, const CFGArg*> m_sub_args;
};

#endif
