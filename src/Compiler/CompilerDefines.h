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

#include <QtGlobal>

namespace FOEDAG {

enum UserAction {
  None,
  Run,
  Settings,
};

static constexpr uint IP_GENERATE{0};
static constexpr uint SYNTHESIS{1};
static constexpr uint SYNTHESIS_SETTINGS{2};
static constexpr uint SYNTHESIS_WRITE_NETLIST{3};
static constexpr uint SYNTHESIS_TIMING_REPORT{4};
static constexpr uint PLACEMENT{5};
static constexpr uint PLACEMENT_SETTINGS{6};
static constexpr uint PLACEMENT_WRITE_NETLIST{7};
static constexpr uint PLACEMENT_TIMING_REPORT{8};
static constexpr uint ROUTING{9};
static constexpr uint ROUTING_SETTINGS{10};
static constexpr uint ROUTING_WRITE_NETLIST{11};
static constexpr uint TIMING_SIGN_OFF{12};
static constexpr uint POWER{13};
static constexpr uint BITSTREAM{14};

static constexpr uint UserActionRole = Qt::UserRole + 1;
static constexpr uint ExpandAreaRole = Qt::UserRole + 2;
static constexpr uint RowVisibilityRole = Qt::UserRole + 3;
static constexpr uint ParentDataRole = Qt::UserRole + 4;

}  // namespace FOEDAG
