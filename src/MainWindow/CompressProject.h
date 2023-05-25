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

#include "Dialog.h"

// include order issue: <filesystem> must go after Dialog.h
#include <filesystem>
namespace fs = std::filesystem;

namespace FOEDAG {

class CompressProject : public Dialog {
  Q_OBJECT

 public:
  explicit CompressProject(const fs::path& project, QWidget* parent = nullptr);

 private slots:
  void compressProject();
  void extensionHasChanged(bool checked);

 private:
  bool ExecuteSystemCommand(const std::string& command,
                            const std::vector<std::string>& args,
                            const std::string& workingDir);

 private:
  QString m_extension;
  const fs::path m_projectPath;
};
}  // namespace FOEDAG
