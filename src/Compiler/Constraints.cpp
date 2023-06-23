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

void Constraints::reset() {
  m_constraints.erase(m_constraints.begin(), m_constraints.end());
  m_keeps.erase(m_keeps.begin(), m_keeps.end());
}

static std::string getConstraint(uint64_t argc, const char* argv[]) {
  std::string command;
  for (uint64_t i = 0; i < argc; i++) {
    command += std::string(argv[i]) + " ";
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

static std::vector<std::string> constraint_procs = {
    //"write_sdc",
    "current_instance", "set_hierarchy_separator", "check_path_divider",
    "set_units", "check_unit", "unit_prefix_scale", "check_unit_scale",
    "set_cmd_units", "set_unit_values", "all_clocks", "all_inputs",
    "all_outputs", "all_ports_for_direction", "port_members", "all_registers",
    "current_design",
    //"get_cells",
    "filter_insts1",
    //"get_clocks",
    "get_lib_cells", "get_lib_pins", "check_nocase_flag", "get_libs",
    "find_liberty_libraries_matching",
    // "get_nets",
    // "get_pins",
    "filter_pins1",
    // "get_ports",
    "filter_ports1",
    // "create_clock",
    "create_generated_clock", "group_path", "check_exception_pins",
    "set_clock_gating_check", "set_clock_gating_check1", "set_clock_groups",
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

static std::string replaceAll(std::string_view str, std::string_view from,
                              std::string_view to) {
  size_t start_pos = 0;
  std::string result(str);
  while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
    result.replace(start_pos, from.length(), to);
    start_pos += to.length();  // Handles case where 'to' is a substr of 'from'
  }
  return result;
}

void Constraints::registerCommands(TclInterpreter* interp) {
  // SDC constraints
  // https://github.com/The-OpenROAD-Project/OpenSTA/blob/master/tcl/Sdc.tcl
  // Register all SDC commands, extract the "keeps"

  // Checks for the sub-syntax supported by VPR

  auto name_harvesting_sdc_command = [](void* clientData, Tcl_Interp* interp,
                                        int argc, const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    const std::string constraint = getConstraint(argc, argv);
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

  auto create_clock = [](void* clientData, Tcl_Interp* interp, int argc,
                         const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    const std::string constraint = getConstraint(argc, argv);
    constraints->addConstraint(constraint);
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        arg = argv[i];
        if (arg != "{*}") constraints->addKeep(arg);
      } else if (arg == "-period") {
        i++;
      } else if (arg == "-waveform") {
        i++;
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
        Tcl_AppendResult(
            interp,
            strdup(
                (std::string(
                     "ERROR: Illegal option for create_clock, check manual: ") +
                 arg)
                    .c_str()),
            (char*)NULL);

        return TCL_ERROR;
      } else {
        if (arg != "{*}") constraints->addKeep(arg);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("create_clock", create_clock, this, 0);

  auto getter_sdc_command = [](void* clientData, Tcl_Interp* interp, int argc,
                               const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    std::string returnVal = "[";
    returnVal += argv[0];
    for (int i = 1; i < argc; i++) {
      std::string arg = argv[i];
      std::string tmp = replaceAll(arg, "@*@", "{*}");
      if (tmp != "{*}") constraints->addKeep(tmp);
      returnVal += " ";
      returnVal += tmp;
    }
    returnVal += "]";
    Tcl_AppendResult(interp, returnVal.c_str(), (char*)NULL);
    return TCL_OK;
  };
  interp->registerCmd("get_clocks", getter_sdc_command, this, 0);
  interp->registerCmd("get_nets", getter_sdc_command, this, 0);
  interp->registerCmd("get_pins", getter_sdc_command, this, 0);
  interp->registerCmd("get_ports", getter_sdc_command, this, 0);
  interp->registerCmd("get_cells", getter_sdc_command, this, 0);

  // Physical constraints
  auto pin_loc = [](void* clientData, Tcl_Interp* interp, int argc,
                    const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    auto constraint = getConstraint(argc, argv);
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
    constraints->addConstraint(getConstraint(argc, argv));
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
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    constraints->addConstraint(getConstraint(argc, argv));
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      }
    }
    return TCL_OK;
  };
  interp->registerCmd("set_property", set_property, this, 0);

  auto set_clock_pin = [](void* clientData, Tcl_Interp* interp, int argc,
                          const char* argv[]) -> int {
    Constraints* constraints = (Constraints*)clientData;
    if (!verifyTimingLimits(argc, argv)) {
      Tcl_AppendResult(interp, TimingLimitErrorMessage, nullptr);
      return TCL_ERROR;
    }
    constraints->addConstraint(getConstraint(argc, argv));
    for (int i = 0; i < argc; i++) {
      std::string arg = argv[i];
      if (arg == "-name") {
        i++;
        if (std::string(argv[i]) != "{*}") constraints->addKeep(argv[i]);
      }
    }

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
    constraints->addConstraint(getConstraint(argc, argv));
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
    text = replaceAll(text, "[*]", "@*@");
    text = replaceAll(text, "{*}", "@*@");
    int status = Tcl_Eval(interp, text.c_str());
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
