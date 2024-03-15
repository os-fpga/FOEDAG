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

#include <QString>

namespace FOEDAG {

struct CustomLayoutData {
  QString baseName{};
  QString name{};
  int width{};
  int height{};
  QString bram{};
  QString dsp{};
};

class CustomLayoutBuilder {
 public:
  CustomLayoutBuilder(const CustomLayoutData &data,
                      const QString &templateLayout);
  std::pair<bool, QString> testTemplateFile() const;
  std::pair<bool, QString> generateCustomLayout() const;

  std::pair<bool, QString> generateNewDevice(const QString &deviceXml,
                                             const QString &targetDeviceXml,
                                             const QString &baseDevice) const;

  void setCustomLayoutData(const CustomLayoutData &data);

 private:
  CustomLayoutData m_data{};
  QString m_templateLayout{};
};

}  // namespace FOEDAG
