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

#include <QMetaType>

namespace FOEDAG {

static constexpr uint UserActionRole = Qt::UserRole + 1;
static constexpr uint ExpandAreaRole = Qt::UserRole + 2;
static constexpr uint RowVisibilityRole = Qt::UserRole + 3;
static constexpr uint ParentDataRole = Qt::UserRole + 4;
static constexpr uint TaskTypeRole = Qt::UserRole + 5;
static constexpr uint TaskId = Qt::UserRole + 6;
static constexpr uint UserActionCleanRole = Qt::UserRole + 7;

enum ExpandAreaAction {
  Invert,
  Expand,
  Collapse,
};

}  // namespace FOEDAG

Q_DECLARE_METATYPE(FOEDAG::ExpandAreaAction)
