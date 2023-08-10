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

#include <QStringList>
#include <filesystem>
#include <regex>

#include "Utils/StringUtils.h"

namespace FOEDAG {

class FileLoaderOldStructure {
 public:
  explicit FileLoaderOldStructure(const QString &projectFileName);

  std::pair<bool, QString> Migrate() const;

 private:
  static StringVector FilesToRemove(const std::string &projectName);
  static StringVector FilesByRegex(const std::filesystem::path &path,
                                   const std::vector<std::regex> &regexes);
  static StringVector FoldersToRemove(const std::string &projectName);
  static StringVector FilesByExtencion(const std::filesystem::path &path,
                                       const StringVector &extensions);

 private:
  QString m_project{};
};

}  // namespace FOEDAG
