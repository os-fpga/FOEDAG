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

#include <QDialog>
#include <filesystem>
namespace FOEDAG {

struct ProjectInfo {
  QString name;
  QString version;
  QString build;
  QString git_hash;
  QString url;
  QString build_type;
  QString licenseFile{};  // base folder is <build path>/share/foedag/etc/
};

class AboutWidget : public QDialog {
 public:
  explicit AboutWidget(const ProjectInfo &info,
                       const std::filesystem::path &srcPath,
                       QWidget *parent = nullptr);

 private:
  static QString License(const std::filesystem::path &srcDir,
                         const QString &file);
  static QString getTagLine(const std::filesystem::path &srcDir);
};

}  // namespace FOEDAG
