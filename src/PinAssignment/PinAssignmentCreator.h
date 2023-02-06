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

#include <QMap>
#include <QWidget>

namespace FOEDAG {

class ToolContext;
class PinsBaseModel;
class PackagePinsLoader;
class PortsLoader;

struct PinAssignmentData {
  ToolContext *context{nullptr};
  QString pinMapFile{};
  QString target{};
  QStringList commands{};
  QString projectPath{};
  QString pinFile{};
  bool useBallId{false};
};

class PinAssignmentCreator : public QObject {
  Q_OBJECT
 public:
  PinAssignmentCreator(const PinAssignmentData &data,
                       QObject *parent = nullptr);
  QWidget *GetPackagePinsWidget();
  QWidget *GetPortsWidget();
  QString generateSdc() const;
  PinsBaseModel *baseModel() const;
  const PinAssignmentData &data() const;
  void setPinFile(const QString &file);
  void setUseBallId(bool useBallId);

  /*!
   * \brief refresh
   * Reload all data from *.pin file
   */
  void refresh();

  /*!
   * \brief searchPortsFile
   * Search file 'port_info.json' in path \param projectPath.
   * \return full path to the file if it exists otherwise return empty string
   */
  static QString searchPortsFile(const QString &projectPath);
  static void RegisterPackagePinLoader(const QString &device,
                                       PackagePinsLoader *l);
  static void RegisterPortsLoader(const QString &device, PortsLoader *l);

 signals:
  void changed();

 private:
  QWidget *CreateLayoutedWidget(QWidget *main);
  QString searchCsvFile() const;
  QString packagePinHeaderFile(ToolContext *context) const;
  PackagePinsLoader *FindPackagePinLoader(const QString &targetDevice) const;
  PortsLoader *FindPortsLoader(const QString &targetDevice) const;
  void parseConstraints(const QStringList &commands,
                        class PackagePinsView *packagePins,
                        class PortsView *portsView);

 private:
  QWidget *m_portsView{nullptr};
  QWidget *m_packagePinsView{nullptr};
  PinsBaseModel *m_baseModel{nullptr};
  static QMap<QString, PackagePinsLoader *> m_loader;
  static QMap<QString, PortsLoader *> m_portsLoader;
  PinAssignmentData m_data;
};

}  // namespace FOEDAG
