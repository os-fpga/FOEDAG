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

class QString;

namespace FOEDAG {

class TopLevelInterface {
 public:
  virtual ~TopLevelInterface() = default;
  virtual void gui_start(bool showWP) = 0;
  // run argument indicates whether opened project should be run
  // after opening. Delayed opening can be requested to allow the GUI to
  // finish its initialization first.
  virtual void openProject(const QString& projectFile, bool delayedOpen,
                           bool run) = 0;
  virtual bool isRunning() const = 0;
  virtual void ProgressVisible(bool visible) = 0;

  // this function is call after tcl script has finished
  virtual void ScriptFinished() {}
};

}  // namespace FOEDAG
