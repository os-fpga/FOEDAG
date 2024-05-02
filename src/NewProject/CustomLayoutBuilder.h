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

struct EFpga {
  double aspectRatio{1.0};
  int bram{0};
  int dsp{0};
  int le{0};
  int fle{0};
  int clb{0};
};

struct CustomLayoutData {
  QString baseName{};
  QString name{};
  EFpga eFpga{};
};

class EFpgaMath {
 public:
  EFpgaMath(const EFpga &fpga) : m_eFpga(fpga) {}

  int widthMultiple() const;
  int heightMultiple() const;
  double phisicalClb() const;
  double grids() const;
  int width() const;
  int height() const;
  int dspCol() const;
  int bramCol() const;
  int clbCol() const;
  int columns() const;
  int colSep() const;
  int colSepmid() const;

  int dspCount() const;
  int bramCount() const;
  int clbCount() const;

  double need() const;
  double actual() const;

  bool isBlockCountValid() const;
  bool isLutCountValid() const;
  bool isDeviceSizeValid() const;

  int lutCount() const;
  int ffCount() const;
  int carryLengthCount() const;

  std::vector<int> dspColumns() const;
  std::vector<int> bramColumns() const;

 private:
  enum Blocks { CLB, BRAM, DSP };
  /*!
   * \brief layoutHalf
   * \return half of the layput starting with the middle position and go to the
   * end of the layout. Need to flip for full layout.
   */
  std::vector<int> layoutHalf() const;

  /*!
   * \brief layoutFor
   * \return vector of real layput indexes for \param block
   */
  std::vector<int> layoutFor(Blocks block) const;

 private:
  EFpga m_eFpga{};
};

class CustomLayoutBuilder {
 public:
  CustomLayoutBuilder(const CustomLayoutData &data,
                      const QString &templateLayout);
  std::pair<bool, QString> testTemplateFile() const;
  std::pair<bool, QString> generateCustomLayout() const;

  std::pair<bool, QString> saveCustomLayout(
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
                        const EFpga &deviceResources) const;

 private:
  CustomLayoutData m_data{};
  QString m_templateLayout{};
};

}  // namespace FOEDAG
