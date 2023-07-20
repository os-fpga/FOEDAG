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
#include <QSettings>

#include "ProgrammerBackend.h"
#include "SummaryProgressBar.h"

namespace Ui {
class ProgrammerMain;
}
class QTreeWidgetItem;
class QProgressBar;

namespace FOEDAG {

enum Status { None, InProgress, Pending, Done };

struct DeviceOptions {
  QString file;
  QStringList operations;
  ProgressCallback_ progress;
};

struct DeviceSettings {
  ~DeviceSettings() { delete flash; }
  FoedagDevice device;
  DeviceOptions devOptions;
  DeviceSettings *flash{nullptr};
  bool isFlash{false};
};

class ProgrammerMain : public QMainWindow {
  Q_OBJECT

 public:
  ProgrammerMain(QWidget *parent = nullptr);
  ~ProgrammerMain();

  QString cfgFile() const;
  void setCfgFile(const QString &cfg);

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
  void updateDeviceOperations(bool ok);

 private:
  void updateTable();
  void updateOperationActions(QTreeWidgetItem *item);
  void updateRow(QTreeWidgetItem *item);
  int itemIndex(QTreeWidgetItem *item) const;
  QMenu *prepareMenu(bool flash);
  void cleanup();
  QStringList BuildDeviceRow(const DeviceSettings &dev, int counter);
  QStringList BuildFlashRow(const DeviceSettings &dev);
  bool VerifyDevices();
  void start();
  static QString ToString(Status status);
  void setStatus(DeviceSettings *ds, Status status);
  void openSettingsWindow(int index);

 private:
  static constexpr int FILE_COL{1};
  static constexpr int OPERATIONS_COL{2};
  static constexpr int STATUS_COL{3};
  static constexpr int PROGRESS_COL{4};
  Ui::ProgrammerMain *ui;
  ProgrammerBackend m_backend{};
  QAction *m_progressAction{nullptr};
  QVector<DeviceSettings *> m_deviceSettings;
  QVector<DeviceSettings *> m_deviceTmp;
  QTreeWidgetItem *m_currentItem{nullptr};
  std::atomic_bool stop{false};
  SummaryProgressBar m_mainProgress;
  QMap<QTreeWidgetItem *, DeviceSettings *> m_items;
  QSettings m_settings;
  bool m_programmingDone{true};
  QString m_cfgFile{};
};

}  // namespace FOEDAG
