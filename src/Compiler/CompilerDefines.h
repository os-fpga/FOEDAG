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
#include <qnamespace.h>

#include <QWidget>
#include <QtGlobal>

namespace FOEDAG {

class Compiler;
class TaskManager;

namespace Design {
enum Language {
  BLIF,
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
};
}

Design::Language FromFileType(const QString &type);

// ID of the tasks shouln't be changed since they save to file
static constexpr uint IP_GENERATE{0};
static constexpr uint SYNTHESIS{1};
static constexpr uint SYNTHESIS_CLEAN{2};
static constexpr uint SYNTHESIS_SETTINGS{3};
static constexpr uint SYNTHESIS_WRITE_NETLIST{4};
static constexpr uint SYNTHESIS_TIMING_REPORT{5};
static constexpr uint PACKING{6};
static constexpr uint PACKING_CLEAN{7};
static constexpr uint GLOBAL_PLACEMENT{8};
static constexpr uint GLOBAL_PLACEMENT_CLEAN{9};
static constexpr uint PLACEMENT{10};
static constexpr uint PLACEMENT_CLEAN{11};
static constexpr uint PLACEMENT_SETTINGS{12};
static constexpr uint PLACEMENT_WRITE_NETLIST{13};
static constexpr uint PLACEMENT_TIMING_REPORT{14};
static constexpr uint ROUTING{15};
static constexpr uint ROUTING_CLEAN{16};
static constexpr uint ROUTING_SETTINGS{17};
static constexpr uint ROUTING_WRITE_NETLIST{18};
static constexpr uint TIMING_SIGN_OFF{19};
static constexpr uint POWER{20};
static constexpr uint BITSTREAM{21};

static constexpr uint UserActionRole = Qt::UserRole + 1;
static constexpr uint ExpandAreaRole = Qt::UserRole + 2;
static constexpr uint RowVisibilityRole = Qt::UserRole + 3;
static constexpr uint ParentDataRole = Qt::UserRole + 4;
static constexpr uint TaskTypeRole = Qt::UserRole + 5;

/*!
 * \brief prepareCompilerView
 * Create app parts of the compiler task view.
 * \param compiler
 * \param taskManager - output parameter to receive pointer to task manager.
 * \return widget with compiler task view
 */
QWidget *prepareCompilerView(Compiler *compiler,
                             TaskManager **taskManager = nullptr);

uint toTaskId(int action, const Compiler *const compiler);

}  // namespace FOEDAG
