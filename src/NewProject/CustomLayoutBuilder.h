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

#include <QDomElement>
#include <QString>
#include <filesystem>

namespace FOEDAG {

struct CustomLayoutData {
  QString baseName{};
  QString name{};
  int width{};
  int height{};
  QString bram{};
  QString dsp{};
};

class CustomDeviceResources {
 public:
  explicit CustomDeviceResources(const CustomLayoutData &data);
  int lutsCount() const;
  int ffsCount() const;
  int bramCount() const;
  int dspCount() const;
  int carryAdderCount() const;

  bool isValid() const;

 private:
  int m_width{};
  int m_height{};
  int m_bramColumnCount{};
  int m_dspColumnCount{};
  static const int bramConst{3};
  static const int dspConst{3};
};

class CustomLayoutBuilder {
 public:
  CustomLayoutBuilder(const CustomLayoutData &data,
                      const QString &templateLayout);
  std::pair<bool, QString> testTemplateFile() const;
  std::pair<bool, QString> generateCustomLayout() const;

  static std::pair<bool, QString> saveCustomLayout(
      const std::filesystem::path &basePath, const QString &fileName,
      const QString &content);

  std::pair<bool, QString> generateNewDevice(const QString &deviceXml,
                                             const QString &targetDeviceXml,
                                             const QString &baseDevice) const;

  /*!
   * \brief modifyDevice - update device name, device_size field and base_device
   * field \param targetDeviceXml - path to custom device file \param modifyDev
   * - name of the device to be modified. \param deviceListFile - load base name
   * from this file. \return true if success and empty message or false and
   * error message
   */
  std::pair<bool, QString> modifyDevice(const QString &targetDeviceXml,
                                        const QString &modifyDev) const;

  static std::pair<bool, QString> removeDevice(
      const QString &deviceXml, const std::filesystem::path &layoutsPath,
      const QString &device);

  /*!
   * \brief fromFile - build CustomLayoutData from file \a file.
   * \param file - data will be load from this file
   * \param data - output parament for data loaded from file.
   * \return true and empty string if data loaded successfully otherwise return
   * false and error string
   */
  static std::pair<bool, QString> fromFile(const QString &file,
                                           const QString &deviceListFile,
                                           CustomLayoutData &data);

 private:
  void modifyDeviceData(const QDomElement &e,
                        const CustomDeviceResources &deviceResources) const;

 private:
  CustomLayoutData m_data{};
  QString m_templateLayout{};
};

}  // namespace FOEDAG
