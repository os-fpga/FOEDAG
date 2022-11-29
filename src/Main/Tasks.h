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

#ifndef TASKS_H
#define TASKS_H

#include <QDialog>

namespace FOEDAG {

class ITaskReportManager;

QDialog* createTaskDialog(const QString& taskName);
void handleTaskDialogRequested(const QString& category);
void handleViewFileRequested(const QString& filePath);
void handleViewReportRequested(ITaskReportManager& reportManager);

// Setters/Getters for tclArgs
void TclArgs_setSynthesisOptions(const std::string& argsStr);
std::string TclArgs_getSynthesisOptions();
void TclArgs_setExampleArgs(const std::string& argsStr);
std::string TclArgs_getExampleArgs();
void TclArgs_setPlacementOptions(const std::string& argsStr);
std::string TclArgs_getPlacementOptions();

}  // namespace FOEDAG

#endif
