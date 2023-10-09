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

#include "MainWindow/TopLevelInterface.h"
#include "ProgrammerGuiCommon.h"
#include "SummaryProgressBar.h"

namespace Ui {
class ProgrammerMain;
}
class QTreeWidgetItem;
class QProgressBar;
class QComboBox;

namespace FOEDAG {

class ProgrammerGuiIntegration;

enum Status { None, InProgress, Pending, Done, Failed };

class ProgrammerMain : public QMainWindow, public TopLevelInterface {
  Q_OBJECT

 public:
  explicit ProgrammerMain(QWidget *parent = nullptr);
  ~ProgrammerMain() override;
  void gui_start(bool showWP) override;
  void openProject(const QString &projectFile, bool delayedOpen,
                   bool run) override {}
  bool isRunning() const override;
  void ProgressVisible(bool visible) override {}

 signals:
  void appendOutput(const QString &);
  void updateProgress(QProgressBar *progressBar, int value);

 protected:
  void closeEvent(QCloseEvent *e) override;

 private slots:
  void onCustomContextMenu(const QPoint &point);
  void startPressed();
  void stopPressed();
  void addFile();
  void reset();
  void showToolTip();
  void updateDeviceOperations(bool ok);
  void progressChanged(const DeviceEntity &entity, const std::string &progress);
  void programStarted(const DeviceEntity &entity);
  void GetDeviceList();
  void autoDetect();
  void itemHasChanged(QTreeWidgetItem *item, int column);
  void updateStatus(const DeviceEntity &entity, int status);

 private:
  void updateTable();
  void updateOperationActions(QTreeWidgetItem *item);
  static void updateRow(QTreeWidgetItem *item, DeviceInfo *deviceInfo);
  int itemIndex(QTreeWidgetItem *item) const;
  QMenu *prepareMenu(bool flash);
  void cleanupStatusAndProgress();
  void cleanDeviceList();
  static QStringList BuildDeviceRow(const DeviceInfo &dev, int counter);
  static QStringList BuildFlashRow(const DeviceInfo &dev);
  bool VerifyDevices();
  void start();
  static QString ToString(Status status);
  void setStatus(DeviceInfo *deviceInfo, Status status);
  void openSettingsWindow(int index);
  static bool EvalCommand(const QString &cmd);
  static bool EvalCommand(const std::string &cmd);
  void SetFile(DeviceInfo *device, const QString &file, bool otp);
  static QString ToString(const QString &str);
  static QString ToString(const QStringList &strList, const QString &sep);
  void loadFromSettigns();
  bool IsEnabled(DeviceInfo *deviceInfo) const;
  static QColor TextColor(Status status);

 private:
  static constexpr int TITLE_COL{0};
  static constexpr int FILE_COL{1};
  static constexpr int OPERATIONS_COL{2};
  static constexpr int STATUS_COL{3};
  static constexpr int PROGRESS_COL{4};
  static constexpr int Frequency{1000};
  static constexpr Qt::CheckState DefaultCheckState{Qt::Unchecked};
  Ui::ProgrammerMain *ui;
  QAction *m_progressAction{nullptr};
  QVector<DeviceInfo *> m_deviceSettings;
  QTreeWidgetItem *m_currentItem{nullptr};
  std::atomic_bool stop{false};
  SummaryProgressBar m_mainProgress;
  QMap<QTreeWidgetItem *, DeviceInfo *> m_items;
  QSettings m_settings;
  bool m_programmingDone{true};
  ProgrammerGuiIntegration *m_guiIntegration;
  QComboBox *m_hardware;
  QComboBox *m_iface;
  QMap<QString, uint64_t> m_frequency{};
};

}  // namespace FOEDAG
