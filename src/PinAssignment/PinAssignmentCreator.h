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
#include <QList>
#include <QWidget>

#ifndef UPSTREAM_PINPLANNER
namespace FOEDAG {
class PcfValidator;
class ErrorsView;
}
#include <QFile>
#endif

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
  QString portsFilePath{};
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
#ifndef UPSTREAM_PINPLANNER
  std::pair<QString, bool> generatePcf() const;
  void validateStoredPcfFile() const;
  static void readPcfCommands(QFile& file, QList<QString>& commands);
#endif
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
#ifdef UPSTREAM_PINPLANNER
  QString packagePinHeaderFile(ToolContext *context) const;
#endif
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
#ifndef UPSTREAM_PINPLANNER
  PcfValidator* m_pcfValidator{nullptr};
  QList<ErrorsView*> m_errorsViews;
  QList<QWidget*> m_ioViews;
#endif
};

}  // namespace FOEDAG
