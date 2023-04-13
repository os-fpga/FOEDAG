/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

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
#pragma once

#include <QTableView>

namespace FOEDAG {

class Compiler;
class TaskManager;

namespace Design {

// The ID's of enum values are important since they are saved to project file.
// Please append new values at the end
enum Language {
  BLIF = 0,
  EBLIF,
  VHDL_1987,
  VHDL_1993,
  VHDL_2000,
  VHDL_2008,
  VERILOG_1995,
  VERILOG_2001,
  VERILOG_NETLIST,
  SYSTEMVERILOG_2005,
  SYSTEMVERILOG_2009,
  SYSTEMVERILOG_2012,
  SYSTEMVERILOG_2017,
  C,
  CPP,
  VHDL_2019,
  EDIF
};
}  // namespace Design

Design::Language FromFileType(const QString &type, bool postSynth = false);

// ID of the tasks shouln't be changed since they save to file
static constexpr uint IP_GENERATE{0};
static constexpr uint SYNTHESIS{1};
static constexpr uint SYNTHESIS_CLEAN{2};
static constexpr uint SYNTHESIS_SETTINGS{3};
// static constexpr uint SYNTHESIS_WRITE_NETLIST{4}; // ID deprecated by RG-230
// static constexpr uint SYNTHESIS_TIMING_REPORT{5}; // ID deprecated by RG-230
static constexpr uint PACKING{6};
static constexpr uint PACKING_CLEAN{7};
static constexpr uint GLOBAL_PLACEMENT{8};
static constexpr uint GLOBAL_PLACEMENT_CLEAN{9};
static constexpr uint PLACEMENT{10};
static constexpr uint PLACEMENT_CLEAN{11};
static constexpr uint PLACEMENT_SETTINGS{12};
// static constexpr uint PLACEMENT_WRITE_NETLIST{13}; // ID deprecated by RG-230
// static constexpr uint PLACEMENT_TIMING_REPORT{14}; // ID deprecated by RG-230
static constexpr uint ROUTING{15};
static constexpr uint ROUTING_CLEAN{16};
static constexpr uint ROUTING_SETTINGS{17};
// static constexpr uint ROUTING_WRITE_NETLIST{18}; // ID deprecated by RG-230
static constexpr uint TIMING_SIGN_OFF{19};
static constexpr uint POWER{20};
static constexpr uint BITSTREAM{21};
static constexpr uint PLACE_AND_ROUTE_VIEW{22};
static constexpr uint ANALYSIS{23};
static constexpr uint ANALYSIS_CLEAN{24};
static constexpr uint BITSTREAM_CLEAN{25};
static constexpr uint POWER_CLEAN{26};
static constexpr uint TIMING_SIGN_OFF_CLEAN{27};
static constexpr uint SIMULATE_RTL{28};
static constexpr uint SIMULATE_RTL_CLEAN{29};
static constexpr uint SIMULATE_RTL_SETTINGS{30};
static constexpr uint SIMULATE_GATE{31};
static constexpr uint SIMULATE_GATE_CLEAN{32};
static constexpr uint SIMULATE_GATE_SETTINGS{33};
static constexpr uint SIMULATE_PNR{34};
static constexpr uint SIMULATE_PNR_CLEAN{35};
static constexpr uint SIMULATE_PNR_SETTINGS{36};
static constexpr uint SIMULATE_BITSTREAM{37};
static constexpr uint SIMULATE_BITSTREAM_CLEAN{38};
static constexpr uint SIMULATE_BITSTREAM_SETTINGS{39};
static constexpr uint TIMING_SIGN_OFF_SETTINGS{40};
static constexpr uint PACKING_SETTINGS{41};

static constexpr const char *IP_GENERATE_LOG{"ip_generate.rpt"};
static constexpr const char *ANALYSIS_LOG{"analysis.rpt"};
static constexpr const char *SYNTHESIS_LOG{"synthesis.rpt"};
static constexpr const char *PACKING_LOG{"packing.rpt"};
static constexpr const char *GLOBAL_PLACEMENT_LOG{"global_placement.rpt"};
static constexpr const char *PLACEMENT_LOG{"placement.rpt"};
static constexpr const char *PLACEMENT_TIMING_LOG{"post_place_timing.rpt"};
static constexpr const char *ROUTING_LOG{"routing.rpt"};
static constexpr const char *ROUTING_TIMING_LOG{"post_route_timing.rpt"};
static constexpr const char *TIMING_ANALYSIS_LOG{"timing_analysis.rpt"};
static constexpr const char *TA_TIMING_LOG{"post_ta_timing.rpt"};
static constexpr const char *POWER_ANALYSIS_LOG{"power_analysis.rpt"};
static constexpr const char *BITSTREAM_LOG{"bitstream.rpt"};

static constexpr const char *SYNTH_SETTING_KEY{"Synthesis"};
static constexpr const char *PLACE_SETTING_KEY{"Placement"};
static constexpr const char *ROUTE_SETTING_KEY{"Routing"};
static constexpr const char *PACKING_SETTING_KEY{"Packing"};
static constexpr const char *TIMING_SETTING_KEY{"Timing Analysis"};
static constexpr const char *SIM_RTL_SETTING_KEY{"Simulate RTL"};
static constexpr const char *SIM_GATE_SETTING_KEY{"Simulate Gate"};
static constexpr const char *SIM_PNR_SETTING_KEY{"Simulate PNR"};
static constexpr const char *SIM_BITSTREAM_SETTING_KEY{"Simulate Bitstream"};

/*!
 * \brief prepareCompilerView
 * Create app parts of the compiler task view.
 * \param compiler
 * \param taskManager - output parameter to receive pointer to task manager.
 * \return widget with compiler task view
 */
class TaskTableView *prepareCompilerView(Compiler *compiler,
                                         TaskManager **taskManager = nullptr);

uint toTaskId(int action, Compiler *const compiler);

/*!
 * \brief read_sdc
 * Run TCL read_sdc command for file \a file.
 * \return 0 if tcl command success otherwise return -1
 */
[[nodiscard]] int read_sdc(const QString &file);
bool target_device(const QString &target);

}  // namespace FOEDAG
