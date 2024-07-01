/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include "Compiler/Constraints.h"

#include "Compiler/Compiler.h"
#include "Configuration/CFGCommon/CFGCommon.h"
#include "DesignQuery/DesignQuery.h"
#include "MainWindow/Session.h"
#include "Utils/StringUtils.h"

using namespace FOEDAG;

constexpr auto TimingLimitErrorMessage{"Invalid setting for -period: 0.1 1000"};

Constraints::Constraints(Compiler* compiler) : m_compiler(compiler) {
  m_interp = new TclInterpreter("");
  registerCommands(m_interp);
}

Constraints::~Constraints() {}

bool Constraints::evaluateConstraints(const std::filesystem::path& path) {
  m_interp->evalFile(path.string());
  return true;
}

bool Constraints::evaluateConstraint(const std::string& constraint) {
  m_interp->evalCmd(constraint);
  return true;
}

const std::string Constraints::SafeParens(const std::string& name) {
  std::string result = name;
  std::size_t bpos = name.find("[");
  std::size_t apos = name.find("@");
  std::size_t cpos = name.find(".");
  std::size_t ppos = name.find("{");
  if ((bpos != std::string::npos || apos != std::string::npos) &&
      cpos != std::string::npos && bpos != 0 && ppos != 0) {
    // If a [ character is part of the string and is not the first
    // character, then it is a complex signal name. ie: FOO[0].bar ->
    // {FOO[0].bar}
    result = "{" + name + "}";
  }
  return result;
}

void Constraints::reset() {
  m_constraints.erase(m_constraints.begin(), m_constraints.end());
  m_keeps.erase(m_keeps.begin(), m_keeps.end());
  m_virtualClocks.clear();
  m_object_properties.clear();
}

std::string Constraints::getConstraint(uint64_t argc, const char* argv[]) {
  std::string command;
  for (uint64_t i = 0; i < argc; i++) {
    command += std::string(
                   GetCompiler()->getNetlistEditData()->PIO2InnerNet(argv[i])) +
               " ";
  }
  return command;
}

static bool verifyTimingLimits(uint64_t argc, const char* argv[]) {
  for (uint64_t i = 0; i < argc; i++) {
    std::string command = std::string(argv[i]);
    if (command == "create_clock") {
      for (uint64_t j = i + 1; j < argc - 1; j++) {
        command = std::string(argv[j]);
        if (command == "-period") {
          auto value = std::string(argv[j + 1]);
          double period = std::stod(value);
          if ((period < 0.1) || (period > 1000)) return false;
        }
      }
    }
  }
  return true;
}

bool Constraints::verify_mode_property(int argc, const char* argv[],
                                       std::string& oldMode) {
  oldMode.clear();
  if (argc < 4 or !argv) return true;
  const char* cmd = argv[0];
  if (::strcmp(cmd, "set_property") != 0) return true;
  if (::strcmp(argv[1], "mode") != 0) return true;

  std::string mode = argv[2];
  std::string gbox = argv[3];
  if (mode.empty() or gbox.empty()) return true;

  auto node = m_gbox2mode.find(gbox);
  if (node == m_gbox2mode.end()) {
    m_gbox2mode.emplace(gbox, mode);
    return true;
  }

  if (mode == node->second) return true;
  oldMode = node->second;
  return false;
}

static std::vector<std::string> constraint_procs = {
    //"write_sdc",
    "current_instance", "set_hierarchy_separator", "check_path_divider",
    "set_units", "check_unit", "unit_prefix_scale", "check_unit_scale",
    "set_cmd_units", "set_unit_values", "all_clocks", /* "all_inputs",
     "all_outputs",*/
    "all_ports_for_direction", "port_members", "all_registers",
    "current_design",
    // "get_cells",
    "filter_insts1",
    // "get_clocks",
    "get_lib_cells", "get_lib_pins", "check_nocase_flag", "get_libs",
    "find_liberty_libraries_matching",
    // "get_nets",
    // "get_pins",
    "filter_pins1",
    // "get_ports",
    "filter_ports1",
    // "create_clock",
    // "create_generated_clock",
    "group_path", "check_exception_pins", "set_clock_gating_check",
    "set_clock_gating_check1",
    // "set_clock_groups",
    "set_clock_latency", "set_sense", "set_clock_sense", "set_clock_sense_cmd1",
    "set_clock_transition", "set_clock_uncertainty", "set_data_check",
    "set_disable_timing", "set_disable_timing_instance",
    "parse_disable_inst_ports", "set_disable_timing_cell",
    "parse_disable_cell_ports", "set_false_path", "set_ideal_latency",
    "set_ideal_network", "set_ideal_transition", "set_input_delay",
    "set_port_delay", "set_max_delay", "set_path_delay", "set_max_time_borrow",
    "set_min_delay", "set_min_pulse_width", "set_multicycle_path",
    "set_output_delay", "set_propagated_clock", "set_case_analysis",
    "set_drive", "set_driving_cell", "set_fanout_load", "set_input_transition",
    "set_load", "set_logic_dc", "set_logic_value", "set_logic_one",
    "set_logic_zero", "set_max_area", "set_max_capacitance",
    "set_capacitance_limit", "set_max_fanout", "set_fanout_limit",
    "set_max_transition", "set_port_fanout_number", "set_resistance",
    "set_timing_derate", "parse_from_arg", "parse_thrus_arg", "parse_to_arg",
    "parse_to_arg1", "delete_from_thrus_to", "parse_comment_key",
    "set_min_capacitance", "set_operating_conditions", "parse_op_cond",
    "parse_op_cond_analysis_type", "set_wire_load_min_block_size",
    "set_wire_load_mode", "set_wire_load_model",
    "set_wire_load_selection_group", "create_voltage_area",
    "set_level_shifter_strategy", "set_level_shifter_threshold",
    "set_max_dynamic_power", "set_max_leakage_power", "define_corners",
    "set_pvt", "set_pvt_min_max", "default_operating_conditions", "cell_regexp",
    "cell_regexp_hsc", "port_regexp", "port_regexp_hsc"};

static std::string get_rid_array_name(const std::string& name) {
  std::string final_name = name;
  uint32_t tracking = 0;
  size_t index = final_name.size() - 1;
  for (auto c = final_name.rbegin(); c < final_name.rend(); c++, index--) {
    if (tracking == 0) {
      if (*c == '%') {
        tracking++;
      } else {
        break;
      }
    } else if (tracking == 1) {
      if (*c >= '0' && *c <= '9') {
        tracking++;
      } else {
        break;
      }
    } else if (tracking == 2) {
      if (*c >= '0' && *c <= '9') {
        continue;
      } else if (*c == '@') {
        tracking++;
        break;
      } else {
        break;
      }
    }
  }
  if (tracking == 3 && index > 0) {
    final_name[index] = '[';
    final_name[final_name.size() - 1] = ']';
  }
  return final_name;
}

const std::string Constraints::ExpandGetters(const std::string& fcall) {
  std::string args;
  {
    std::string get_clocks = "[get_clocks ";
    if (fcall.find(get_clocks) != std::string::npos) {
      args = fcall.substr(std::size(get_clocks),
                          fcall.size() - std::size(get_clocks) - 1);
      return ExpandGetClocks(args);
    }
  }
  {
    std::string get_nets = "[get_nets ";
    if (fcall.find(get_nets) != std::string::npos) {
      args = fcall.substr(std::size(get_nets),
                          fcall.size() - std::size(get_nets) - 1);
      return ExpandGetNets(args);
    }
  }
  {
    std::string get_pins = "[get_pins ";
    if (fcall.find(get_pins) != std::string::npos) {
      args = fcall.substr(std::size(get_pins),
                          fcall.size() - std::size(get_pins) - 1);
      return ExpandGetNets(args);
    }
  }
  {
    std::string get_ports = "[get_ports ";
    if (fcall.find(get_ports) != std::string::npos) {
      args = fcall.substr(std::size(get_ports),
                          fcall.size() - std::size(get_ports) - 1);
      return ExpandGetNets(args);
    }
  }
  return fcall;
}

const std::string Constraints::ClockIsDerivedFrom(const std::string& name) {
  auto itr = m_clockDerivedFromMap.find(name);
  if (itr != m_clockDerivedFromMap.end()) {
    return (*itr).second;
  }
  return "";
}

// TODO: expand regexp
const std::string Constraints::ExpandGetClocks(const std::string& name) {
  return name;
}

// TODO: expand regexp
const std::string Constraints::ExpandGetNets(const std::string& name) {
  return name;
}

// TODO: expand regexp
const std::string Constraints::ExpandGetPins(const std::string& name) {
  return name;
}

// TODO: expand regexp
const std::string Constraints::ExpandGetPorts(const std::string& name) {
  return name;
}

void Constraints::registerCommands(TclInterpreter* interp) {
  // SDC constraints
  // https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/tcl/Sdc.tcl
  // Register all SDC commands, extract the "keeps"

  // Checks for the sub-syntax supported by VPR

  auto name_harvesting_sdc_command = [](void* clientData, Tcl_Interp* interp,
                                        int argc, const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    const std::string constraint = constraints->getConstraint(argc, argv);
    constraints->addConstraint(constraint);
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-clock" || arg == "-name" || arg == "-from" || arg == "-to" ||
          arg == "-through" || arg == "-fall_to" || arg == "-rise_to" ||
          arg == "-rise_from" || arg == "-fall_from") {
        i++;
        arg = argv[i];
        if (arg != "{*}") constraints->addKeep(arg);
      }
    }
    return TCL_OK;
  };
  for (auto proc_name : constraint_procs) {
    interp->registerCmd(proc_name, name_harvesting_sdc_command, this, 0);
  }

  auto create_generated_clock = [](void* clientData, Tcl_Interp* interp,
                                   int argc, const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    if (constraints->GetPolicy() == ConstraintPolicy::SDC) {
      std::string constraint = constraints->getConstraint(argc, argv);
      constraints->addConstraint(constraint);
      return TCL_OK;
    }
    std::string constraint = std::string("create_clock") + " ";
    std::string actual_clock;
    std::string master_clock;
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
      } else if (arg == "-source") {
        i++;
        master_clock = constraints->ExpandGetters(argv[i]);
        master_clock =
            constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                master_clock);
        auto masterClockData =
            constraints->getClockPeriodMap().find(master_clock);
        if (masterClockData == constraints->getClockPeriodMap().end()) {
          Tcl_AppendResult(
              interp,
              ("create_generated_clock, unknown source clock pin: " +
               master_clock)
                  .c_str(),
              nullptr);
          return TCL_ERROR;
        }
      } else if (arg == "-edges") {
        i++;
      } else if (arg == "-divide_by") {
        i++;
      } else if (arg == "-multiply_by") {
        i++;
      } else if (arg == "-combinational") {
      } else if (arg == "-duty_cycle") {
        i++;
      } else if (arg == "-invert") {
      } else if (arg == "-edge_shift") {
        i++;
      } else if (arg == "-add") {
        i++;
      } else if (arg == "-master_clock") {
        i++;
        master_clock =
            constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                argv[i]);
        auto masterClockData =
            constraints->getClockPeriodMap().find(master_clock);
        if (masterClockData == constraints->getClockPeriodMap().end()) {
          Tcl_AppendResult(
              interp,
              ("create_generated_clock, unknown master clock: " + master_clock)
                  .c_str(),
              nullptr);
          return TCL_ERROR;
        }
      } else if (arg == "-quiet") {
      } else if (arg == "-verbose") {
      } else if (arg.find("-") != std::string::npos) {
        i++;
      } else if (arg.find("[") != std::string::npos) {
        actual_clock = constraints->ExpandGetters(argv[i]);
        constraints->addKeep(actual_clock);
        i++;
        if (constraints->GetCompiler()->CompilerState() ==
            Compiler::State::Synthesized) {
          auto [isRtlClock, message] =
              constraints->GetCompiler()->isRtlClock(actual_clock, false);
          if (!isRtlClock && !message.empty()) {
            Tcl_AppendResult(interp, message.c_str(), nullptr);
          }
          if (!isRtlClock) {
            Tcl_AppendResult(
                interp,
                std::string(
                    "ERROR: Generated clocks have to be "
                    "internal design nets driven by a FCLK_BUF primitive, \"" +
                    actual_clock + "\" is not.")
                    .c_str(),
                nullptr);
            return TCL_ERROR;
          }
        }
      } else {
        if (arg != "{*}") {
          actual_clock =
              constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                  arg);
          constraints->addKeep(actual_clock);
          if (constraints->GetCompiler()->CompilerState() ==
              Compiler::State::Synthesized) {
            auto [isRtlClock, message] =
                constraints->GetCompiler()->isRtlClock(actual_clock, false);
            if (!isRtlClock && !message.empty()) {
              Tcl_AppendResult(interp, message.c_str(), nullptr);
            }
            if (!isRtlClock) {
              Tcl_AppendResult(interp,
                               std::string("ERROR: Generated clocks have to be "
                                           "internal design nets driven by a "
                                           "FCLK_BUF primitive, \"" +
                                           actual_clock + "\" is not.")
                                   .c_str(),
                               nullptr);
              return TCL_ERROR;
            }
          }
        }
      }
    }

    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        Tcl_AppendResult(interp,
                         "In SDC compatibility mode, create_generated_clock "
                         "does not accept -name <clk> option",
                         nullptr);
        return TCL_ERROR;
      } else if (arg == "-source") {
        i++;
      } else if (arg == "-edges") {
        i++;
      } else if (arg == "-divide_by") {
        i++;
        arg = argv[i];
        float divide_by = std::stof(arg);
        auto masterClockData =
            constraints->getClockPeriodMap().find(master_clock);
        if (masterClockData != constraints->getClockPeriodMap().end()) {
          float period = (*masterClockData).second / divide_by;
          constraint += "-period ";
          constraint += std::to_string(period) + " ";
          constraints->getClockPeriodMap().emplace(actual_clock, period);
          constraints->getClockDerivedMap().emplace(actual_clock, master_clock);
        }
      } else if (arg == "-multiply_by") {
        i++;
        arg = argv[i];
        float multiply_by = std::stof(arg);
        auto masterClockData =
            constraints->getClockPeriodMap().find(master_clock);
        if (masterClockData != constraints->getClockPeriodMap().end()) {
          float period = (*masterClockData).second * multiply_by;
          constraint += "-period ";
          constraint += std::to_string(period) + " ";
          constraints->getClockPeriodMap().emplace(actual_clock, period);
        }
      } else if (arg == "-combinational") {
      } else if (arg == "-duty_cycle") {
        i++;
      } else if (arg == "-invert") {
      } else if (arg == "-edge_shift") {
        i++;
      } else if (arg == "-add") {
        i++;
      } else if (arg == "-master_clock") {
        i++;
      } else if (arg == "-quiet") {
      } else if (arg == "-verbose") {
      } else if (arg.find("-") != std::string::npos) {
        Tcl_AppendResult(
            interp,
            strdup((std::string("ERROR: Illegal option for "
                                "create_generated_clock, check manual: ") +
                    arg)
                       .c_str()),
            (char*)NULL);
        return TCL_ERROR;
      } else if (arg.find("[") != std::string::npos) {
        bool inCommand = true;
        std::string value;
        for (uint32_t i = 1; i < arg.size(); i++) {
          char c = arg[i];
          if (c == ']') {
          } else if (c == ' ') {
            inCommand = false;
          } else if (!inCommand) {
            value += c;
          }
        }
        constraint += value + " ";
      } else {
        if (arg != "{*}") {
          constraint += arg + " ";
          constraints->addKeep(arg);
          if (constraints->GetCompiler()->CompilerState() ==
              Compiler::State::Synthesized) {
            auto [isRtlClock, message] =
                constraints->GetCompiler()->isRtlClock(arg, false);
            if (!isRtlClock && !message.empty()) {
              Tcl_AppendResult(interp, message.c_str(), nullptr);
            }
            if (!isRtlClock) {
              Tcl_AppendResult(interp,
                               std::string("ERROR: Generated clocks have to be "
                                           "internal design nets driven by a "
                                           "FCLK_BUF primitive, \"" +
                                           arg + "\" is not.")
                                   .c_str(),
                               nullptr);
              return TCL_ERROR;
            }
          }
        }
      }
    }
    constraints->addConstraint(constraint);
    return TCL_OK;
  };
  interp->registerCmd("create_generated_clock", create_generated_clock, this,
                      0);

  auto create_clock = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    if (constraints->GetPolicy() == ConstraintPolicy::SDC) {
      std::string constraint = constraints->getConstraint(argc, argv);
      constraints->addConstraint(constraint);
      return TCL_OK;
    }
    std::string constraint = std::string(argv[0]) + " ";
    std::string actual_clock;
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
      } else if (arg == "-period") {
        i++;
      } else if (arg == "-waveform") {
        i++;
      } else if (arg.find("-") != std::string::npos) {
        i++;
      } else if (arg.find("[") != std::string::npos) {
        i++;
      } else {
        if (arg != "{*}") {
          actual_clock =
              constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                  arg);
        }
      }
    }

    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        arg = argv[i];
        if (arg != "{*}") {
          if (constraints->GetPolicy() == ConstraintPolicy::SDCCompatible) {
            // Ignore the -name <clock>, only use the [get_clock <clock>]
            // portion of the command (Names have to match)
            if (actual_clock.empty() ||
                ((!actual_clock.empty()) &&
                 (constraints->GetCompiler()
                      ->getNetlistEditData()
                      ->PIO2InnerNet(arg) == actual_clock))) {
              if (actual_clock.empty()) {
                bool unique = constraints->AddVirtualClock(arg);
                if (!unique) {
                  Tcl_AppendResult(
                      interp,
                      "ERROR: Only one Virtual clock definition is allowed",
                      nullptr);
                  return TCL_ERROR;
                }
                constraint += "-name " +
                              constraints->GetCompiler()
                                  ->getNetlistEditData()
                                  ->PIO2InnerNet(arg) +
                              " ";
              }
              actual_clock = constraints->GetCompiler()
                                 ->getNetlistEditData()
                                 ->PIO2InnerNet(arg);
              continue;
            } else {
              Tcl_AppendResult(
                  interp,
                  "In SDC compatibility mode for: create_clock -name <clk> "
                  "[get_clock/get_port <clk>], <clk> names have to match.",
                  nullptr);
              return TCL_ERROR;
            }
          } else {
            constraint +=
                "-name " +
                constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                    arg) +
                " ";
          }
          bool unique = constraints->AddVirtualClock(arg);
          if (!unique) {
            Tcl_AppendResult(
                interp, "ERROR: Only one Virtual clock definition is allowed",
                nullptr);
            return TCL_ERROR;
          }
          auto [isRtlClock, message] =
              constraints->GetCompiler()->isRtlClock(arg, false);
          if (!isRtlClock && !message.empty()) {
            Tcl_AppendResult(interp, message.c_str(), nullptr);
          }
          if (!isRtlClock) {
            Tcl_AppendResult(interp,
                             "ERROR: Virtual clock \"" + arg +
                                 "\" cannot be one of the RTL "
                                 "design actual ports/clocks",
                             nullptr);
            return TCL_ERROR;
          }
          constraints->addKeep(arg);
        }
      } else if (arg == "-period") {
        i++;
        arg = argv[i];
        constraint += "-period ";
        constraint += arg + " ";
        float period = std::stof(arg);
        constraints->getClockPeriodMap().emplace(actual_clock, period);
      } else if (arg == "-waveform") {
        i++;
        arg = argv[i];
        constraint += "-waveform ";
        constraint += arg + " ";
      } else if (arg.find("-") != std::string::npos) {
        Tcl_AppendResult(
            interp,
            strdup(
                (std::string(
                     "ERROR: Illegal option for create_clock, check manual: ") +
                 arg)
                    .c_str()),
            (char*)NULL);
        return TCL_ERROR;
      } else if (arg.find("[") != std::string::npos) {
        bool inCommand = true;
        std::string value;
        for (uint32_t i = 1; i < arg.size(); i++) {
          char c = arg[i];
          if (c == ']') {
          } else if (c == ' ') {
            inCommand = false;
          } else if (!inCommand) {
            value += c;
          }
        }
        constraint +=
            constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                value) +
            " ";
      } else {
        if (arg != "{*}") {
          auto [isRtlClock, message] =
              constraints->GetCompiler()->isRtlClock(arg, true);
          if (!isRtlClock && !message.empty()) {
            Tcl_AppendResult(interp, message.c_str(), nullptr);
          }
          if (!isRtlClock) {
            if (constraints->GetCompiler()->CompilerState() ==
                Compiler::State::Synthesized) {
              constraints->GetCompiler()->Message(
                  std::string{"ERROR: Clock \""} + arg +
                  "\" has to be a valid design clock. Synthesis could not "
                  "infer this signal to be an actual design clock");
            } else {
              constraints->GetCompiler()->Message(
                  std::string{"ERROR: Clock \""} + arg +
                  "\" has to be one of the RTL design ports");
            }
            return TCL_ERROR;
          }
          constraint +=
              constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                  arg) +
              " ";
          constraints->addKeep(arg);
        }
      }
    }
    constraints->addConstraint(constraint);
    return TCL_OK;
  };
  interp->registerCmd("create_clock", create_clock, this, 0);

  auto set_clock_groups = [](void* clientData, Tcl_Interp* interp, int argc,
                             const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    std::string constraint;
    std::string actual_clock;
    if (constraints->GetPolicy() == ConstraintPolicy::SDC) {
      constraint = constraints->getConstraint(argc, argv);
    } else {
      constraint = std::string(argv[0]) + " ";
      for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-physically_exclusive") {
          constraint += "-exclusive ";
        } else if (arg.find("[") != std::string::npos) {
          bool inCommand = true;
          std::string value;
          for (uint32_t i = 1; i < arg.size(); i++) {
            char c = arg[i];
            if (c == ']') {
            } else if (c == ' ') {
              inCommand = false;
            } else if (!inCommand) {
              value += c;
            }
          }
          constraint +=
              constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                  value) +
              " ";
        } else {
          constraint +=
              constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                  arg) +
              " ";
        }
      }
    }
    constraints->addConstraint(constraint);
    return TCL_OK;
  };
  interp->registerCmd("set_clock_groups", set_clock_groups, this, 0);

  auto getter_sdc_command = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    // Command
    StringVector arguments;
    arguments.push_back(argv[0]);
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      std::string tmp = StringUtils::replaceAll(arg, "@*@", "{*}");
      tmp = constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(tmp);
      if (tmp != "{*}") constraints->addKeep(tmp);
      tmp = constraints->SafeParens(tmp);
      arguments.push_back(tmp);
    }
    std::string returnVal =
        StringUtils::format("[%]", StringUtils::join(arguments, " "));
    Tcl_AppendResult(interp, returnVal.c_str(), (char*)NULL);
    return TCL_OK;
  };

  // get_ports is already defined in DesignQuery
  // TODO: All of the below commands needs to be defined in DesignQuery too.
  interp->registerCmd("get_clocks", getter_sdc_command, this, 0);
  interp->registerCmd("get_nets", getter_sdc_command, this, 0);
  interp->registerCmd("get_pins", getter_sdc_command, this, 0);
  interp->registerCmd("get_cells", getter_sdc_command, this, 0);

  // Physical constraints
  auto pin_loc = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    auto constraint = constraints->getConstraint(argc, argv);
    constraints->addConstraint(constraint);
    if ((argc != 3) && (argc != 4)) {
      Tcl_AppendResult(
          interp,
          strdup(std::string("set_pin_loc command takes 2 or 3 arguments: " +
                             constraint)
                     .c_str()),
          (char*)NULL);
      return TCL_ERROR;
    }
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_pin_loc", pin_loc, this, 0);

  auto set_mode = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    constraints->addConstraint(constraints->getConstraint(argc, argv));
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_mode", set_mode, this, 0);

  auto set_property = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    if (argc < 4) {
      Tcl_AppendResult(interp,
                       "ERROR: Invalid set_property format. Expect\n"
                       "  1. set_property <name> <value> <object>s\n"
                       "     or\n"
                       "  2. set_property -dict <dictionary> <object>s",
                       (char*)NULL);
      return TCL_ERROR;
    }
    Constraints* constraints = (Constraints*)clientData;
#if 0
    for (int i = 0; i < argc; i++) {
      constraints->GetCompiler()->Message(std::string{"CONSTRAINT set_property DEBUG: "} + std::string{argv[i]});
    }
#endif
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    std::string oldMode;
    if (!constraints->verify_mode_property(argc, argv, oldMode)) {
      char msg[1024] = {};
      ::snprintf(
          msg, 1023,
          "Cannot set mode '%s' for gearbox %s; the existing mode is '%s'",
          argv[2], argv[3], oldMode.c_str());
      Tcl_AppendResult(interp, msg, nullptr);
      return TCL_ERROR;
    }
    constraints->addConstraint(constraints->getConstraint(argc, argv));
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      }
    }
    std::vector<PROPERTY> properties;
    std::vector<std::string> objects;
    if (std::string(argv[1]) == "-dict") {
      // This is dictionary
      // Split the word
      std::string dict = CFG_replace_string(argv[2], "\t", " ");
      std::vector<std::string> results = CFG_split_string(dict, " ", 0, false);
      if (results.size() > 0 && (results.size() % 2) == 0) {
        for (size_t i = 0; i < results.size(); i += 2) {
          properties.push_back(PROPERTY(results[i], results[i + 1]));
        }
      } else {
        Tcl_AppendResult(interp,
                         "ERROR: set_property -dict argument expect pair(s) of "
                         "<name> or <value>\n",
                         (char*)NULL);
        return TCL_ERROR;
      }
    } else {
      properties.push_back(PROPERTY(argv[1], argv[2]));
    }
    for (int i = 3; i < argc; i++) {
      // No duplication
      std::string obj = get_rid_array_name(std::string(argv[i]));
      if (CFG_find_string_in_vector(objects, obj) < 0) {
        objects.push_back(obj);
      }
    }
    constraints->set_property(objects, properties);
    return TCL_OK;
  };
  interp->registerCmd("set_property", set_property, this, 0);

  auto clear_property = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    constraints->clear_property();
    return TCL_OK;
  };
  interp->registerCmd("clear_property", clear_property, this, 0);

  auto write_property = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    if (argc < 2) {
      Tcl_AppendResult(interp,
                       "ERROR: Invalid write_property format. Expect "
                       "\"write_property <filepath>\"",
                       (char*)NULL);
      return TCL_ERROR;
    }
    Constraints* constraints = (Constraints*)clientData;
    constraints->write_property(argv[1]);
    return TCL_OK;
  };
  interp->registerCmd("write_property", write_property, this, 0);

  auto write_simplified_property = [](void* clientData, Tcl_Interp* interp,
                                      int argc, const char* argv[]) -> int {
    if (argc < 2) {
      Tcl_AppendResult(
          interp,
          "ERROR: Invalid write_simplified_property format. Expect "
          "\"write_simplified_property <filepath>\"",
          (char*)NULL);
      return TCL_ERROR;
    }
    Constraints* constraints = (Constraints*)clientData;
    constraints->write_simplified_property(argv[1]);
    return TCL_OK;
  };
  interp->registerCmd("write_simplified_property", write_simplified_property,
                      this, 0);

  auto set_clock_pin = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    std::string constraint = std::string(argv[0]) + " ";
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      } else if (arg == "-design_clock") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
        constraint +=
            "-design_clock " +
            constraints->GetCompiler()->getNetlistEditData()->PIO2InnerNet(
                argv[i]) +
            std::string(" ");
      } else if (arg == "-device_clock") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
        constraint +=
            std::string("-device_clock ") + argv[i] + std::string(" ");
      } else {
        Tcl_AppendResult(interp,
                         "ERROR: Invalid set_clock_pin option. Expect "
                         "\"-design_clock <name> -device_clock <clk[n]>\"",
                         (char*)NULL);
        return TCL_ERROR;
      }
    }
    constraints->addConstraint(constraint);
    return TCL_OK;
  };
  interp->registerCmd("set_clock_pin", set_clock_pin, this, 0);

  auto script_path = [](void* clientData, Tcl_Interp* interp, int argc,
                        const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;

    std::filesystem::path script =
        constraints->GetCompiler()->GetSession()->CmdLine()->Script();
    std::filesystem::path scriptPath = script.parent_path();
    Tcl_SetResult(interp, (char*)scriptPath.c_str(), TCL_VOLATILE);
    return TCL_OK;
  };
  interp->registerCmd("script_path", script_path, this, 0);

  auto region_loc = [](void* clientData, Tcl_Interp* interp, int argc,
                       const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    constraints->addConstraint(constraints->getConstraint(argc, argv));
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_region_loc", region_loc, this, 0);

  auto read_sdc = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    if (argc < 2) {
      Tcl_AppendResult(interp, "ERROR: Specify an sdc file", (char*)NULL);
      return TCL_ERROR;
    }
    DesignQuery* designQuery{nullptr};
    Constraints* constr = static_cast<Constraints*>(clientData);
    if (constr) designQuery = constr->GetCompiler()->GetDesignQuery();
    std::string fileName = argv[1];
    std::ifstream stream;
    stream.open(fileName);
    if (!stream.good()) {
      Tcl_AppendResult(
          interp,
          strdup(std::string("ERROR: Cannot open the SDC file:" + fileName)
                     .c_str()),
          (char*)NULL);
      return TCL_ERROR;
    }
    std::string text;
    char c = stream.get();
    while (stream.good()) {
      text += c;
      c = stream.get();
      if (c == '[') {
        c = stream.get();
        if (isdigit(c)) {
          text += "@";
          while (c != ']') {
            text += c;
            c = stream.get();
          }
          c = stream.get();
          text += "%";
        } else {
          text += '[';
        }
      }
    }
    stream.close();
    text = StringUtils::replaceAll(text, "[*]", "@*@");
    text = StringUtils::replaceAll(text, "{*}", "@*@");
    if (designQuery) designQuery->SetReadSdc(true);
    int status = Tcl_Eval(interp, text.c_str());
    if (designQuery) designQuery->SetReadSdc(false);
    if (status) {
      Tcl_Obj* errorDict = Tcl_GetReturnOptions(interp, status);
      Tcl_Obj* errorInfo = Tcl_NewStringObj("-errorinfo", -1);
      Tcl_Obj* errorMsg;
      Tcl_IncrRefCount(errorDict);
      Tcl_DictObjGet(interp, errorDict, errorInfo, &errorMsg);
      Tcl_DecrRefCount(errorDict);
      char* msgString = Tcl_GetString(errorMsg);  // Get stackTrace
      Tcl_DecrRefCount(errorInfo);
      Tcl_ResetResult(interp);
      Tcl_AppendResult(
          interp,
          strdup((std::string("SDC file syntax error ") + fileName + ":" +
                  std::to_string(Tcl_GetErrorLine(interp)))
                     .c_str()),
          "\n", msgString, (char*)NULL);
      return TCL_ERROR;
    }
    return TCL_OK;
  };

  interp->registerCmd("read_sdc", read_sdc, this, 0);

  auto write_sdc = [](void* clientData, Tcl_Interp* interp, int argc,
                      const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (argc < 2) {
      Tcl_AppendResult(interp, "ERROR: Specify an SDC file", (char*)NULL);
      return TCL_ERROR;
    }
    std::string fileName = argv[1];
    std::ofstream stream;
    stream.open(fileName);
    if (!stream.good()) {
      Tcl_AppendResult(
          interp,
          strdup(std::string("ERROR: Cannot open the SDC file for writing:" +
                             fileName)
                     .c_str()),
          (char*)NULL);
      return TCL_ERROR;
    }
    for (auto constraint : constraints->getConstraints()) {
      stream << constraint << "\n";
    }
    stream.close();
    return TCL_OK;
  };
  interp->registerCmd("write_sdc", write_sdc, this, 0);
}

bool Constraints::AddVirtualClock(const std::string& vClock) {
  auto it = m_virtualClocks.find(vClock);
  if (it != m_virtualClocks.end()) return false;
  if (m_virtualClocks.size() == 1) return false;
  m_virtualClocks.insert(vClock);
  return true;
}

void Constraints::set_property(std::vector<std::string> objects,
                               std::vector<PROPERTY> properties) {
  m_object_properties.push_back(OBJECT_PROPERTY(objects, properties));
}

void Constraints::clear_property() { reset(); }

const std::vector<OBJECT_PROPERTY> Constraints::get_property() {
  return m_object_properties;
}

nlohmann::json Constraints::get_property_by_json() {
  nlohmann::json instances = nlohmann::json::array();
  for (auto& obj_p : m_object_properties) {
    for (auto obj : obj_p.objects) {
      // find the instance index
      size_t index = (size_t)(-1);
      for (size_t i = 0; i < instances.size(); i++) {
        if (instances[i]["name"] == obj) {
          index = i;
          break;
        }
      }
      if (index == (size_t)(-1)) {
        index = instances.size();
        nlohmann::json instance = nlohmann::json::object();
        instance["name"] = obj;
        instance["defined_properties"] = nlohmann::json::array();
        instance["properties"] = nlohmann::json::object();
        instances.push_back(instance);
      }
      nlohmann::json properties = nlohmann::json::object();
      for (auto p : obj_p.properties) {
        properties[p.name] = p.value;
        instances[index]["properties"][p.name] = p.value;
      }
      instances[index]["defined_properties"].push_back(properties);
    }
  }
  return instances;
}

nlohmann::json Constraints::get_simplified_property_json() {
  nlohmann::json properties = nlohmann::json::object();
  for (auto& obj_p : m_object_properties) {
    for (auto obj : obj_p.objects) {
      if (!properties.contains(obj)) {
        properties[obj] = nlohmann::json::object();
      }
      for (auto property : obj_p.properties) {
        properties[obj][property.name] = property.value;
      }
    }
  }
  return properties;
}

void Constraints::write_property(const std::string& filepath) {
  nlohmann::json instances = get_property_by_json();
  nlohmann::json json = nlohmann::json::object();
  json["instances"] = instances;
  std::ofstream file(filepath);
  file << json.dump(2);
  file.close();
}

void Constraints::write_simplified_property(const std::string& filepath) {
  nlohmann::json properties = get_simplified_property_json();
  std::ofstream file(filepath);
  file << properties.dump(2);
  file.close();
}
