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

#include <QMainWindow>
#include <QMap>

#include "ProgrammerBackend.h"
#include "SummaryProgressBar.h"

namespace Ui {
class ProgrammerMain;
}
class QTreeWidgetItem;
class QProgressBar;

namespace FOEDAG {

struct DeviceOptions {
  QString file;
  QStringList operations;
  ProgressCallback progress;
};

struct DeviceSettings {
  FoedagDevice device;
  DeviceOptions devOptions;
  DeviceOptions flashOptions;
  bool operator==(const DeviceSettings &other) {
    return device.name == other.device.name &&
           devOptions.file == other.devOptions.file &&
           devOptions.operations == other.devOptions.operations &&
           flashOptions.file == other.flashOptions.file &&
           flashOptions.operations == other.flashOptions.operations;
  }
};

class ProgrammerMain : public QMainWindow {
  Q_OBJECT

 public:
  ProgrammerMain(QWidget *parent = nullptr);
  ~ProgrammerMain();

 signals:
  void appendOutput(const QString &);
  void updateProgress(QProgressBar *progressBar, int value);

 protected:
  void closeEvent(QCloseEvent *e) override;

 private slots:
  void onCustomContextMenu(const QPoint &point);
  void autoDetect();
  void startPressed();
  void stopPressed();
  void addFile();
  void reset();
  void showToolTip();
  void updateProgressSlot(QProgressBar *progressBar, int value);

 private:
  void updateTable();
  void updateOperations(QTreeWidgetItem *item);
  DeviceSettings itemToDevice(QTreeWidgetItem *item) const;
  int itemIndex(QTreeWidgetItem *item) const;
  void prepareMenu(bool flash);
  void cleanup();
  QStringList BuildDeviceRow(const DeviceSettings &dev, int counter);
  QStringList BuildFlashRow(const DeviceSettings &dev);
  bool VerifyDevices();
  void start();

 private:
  static constexpr int OPERATIONS_COL{2};
  static constexpr int PROGRESS_COL{4};
  Ui::ProgrammerMain *ui;
  ProgrammerBackend m_backend{};
  QAction *m_progressAction{nullptr};
  QVector<DeviceSettings> m_deviceSettings;
  QVector<DeviceSettings> m_deviceTmp;
  QTreeWidgetItem *current{nullptr};
  std::atomic_bool stop{false};
  SummaryProgressBar m_mainProgress;
};

}  // namespace FOEDAG
