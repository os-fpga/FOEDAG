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

#include "ProgrammerMain.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QToolTip>
#include <iostream>

#include "Console/FileNameParser.h"
#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleBuilder.h"
#include "Console/TclConsoleWidget.h"
#include "Console/TclErrorParser.h"
#include "MainWindow/Session.h"
#include "ProgrammerGuiIntegration.h"
#include "ProgrammerSettingsWidget.h"
#include "ui_ProgrammerMain.h"

inline void InitResources() {
  Q_INIT_RESOURCE(res);
  Q_INIT_RESOURCE(main_window_resource);
}

extern FOEDAG::Session *GlobalSession;

namespace FOEDAG {

static const char *NONE_STR{"-"};

ProgrammerMain::ProgrammerMain(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ProgrammerMain),
      m_settings("programmer_settings", QSettings::IniFormat),
      m_guiIntegration(new ProgrammerGuiIntegration) {
  InitResources();
  ui->setupUi(this);

  Gui::SetGuiInterface(m_guiIntegration);

  QComboBox *hardware = new QComboBox;
  hardware->addItem("FTDI");
  hardware->setFixedWidth(120);

  QLabel *label1 = new QLabel("Hardware:");
  label1->setBuddy(hardware);

  QComboBox *iface = new QComboBox;
  iface->addItem("JTAG");
  iface->setFixedWidth(120);

  QLabel *label2 = new QLabel("Interface:");
  label2->setBuddy(iface);

  m_mainProgress.progressBar()->setAlignment(Qt::AlignHCenter);

  ui->toolBar->insertWidget(ui->actionDetect, label1);
  ui->toolBar->insertWidget(ui->actionDetect, hardware);
  ui->toolBar->insertWidget(ui->actionDetect, label2);
  ui->toolBar->insertWidget(ui->actionDetect, iface);
  m_progressAction = ui->toolBar->addWidget(m_mainProgress.progressBar());
  ui->toolBar->layout()->setSpacing(5);
  ui->toolBar->layout()->setContentsMargins(5, 5, 5, 5);

  ui->treeWidget->header()->resizeSection(0, 150);
  ui->treeWidget->header()->resizeSection(1, 350);
  ui->treeWidget->header()->resizeSection(2, 200);
  ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  ui->treeWidget->expandAll();
  connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this,
          &ProgrammerMain::onCustomContextMenu);
  connect(ui->actionExit, &QAction::triggered, this, &ProgrammerMain::close);
  connect(ui->actionDetect, &QAction::triggered, this,
          &ProgrammerMain::GetDeviceList);
  connect(ui->actionStart, &QAction::triggered, this,
          &ProgrammerMain::startPressed);
  connect(ui->actionStop, &QAction::triggered, this,
          &ProgrammerMain::stopPressed);
  connect(ui->actionAdd_File, &QAction::triggered, this,
          &ProgrammerMain::addFile);
  connect(ui->actionReset, &QAction::triggered, this, &ProgrammerMain::reset);
  ui->toolBar->removeAction(ui->actionStop);
  QIcon image{":/images/help-circle.png"};
  auto btn = new QPushButton{image, {}};
  btn->setFlat(true);
  connect(btn, &QPushButton::clicked, this, &ProgrammerMain::showToolTip);
  // todo @volodymyrk RG-433 hide for now
  //  menuBar()->setCornerWidget(btn);

  connect(this, &ProgrammerMain::updateProgress, this,
          &ProgrammerMain::updateProgressSlot);

  for (QAction *action : {ui->actionBlankcheck, ui->actionErase,
                          ui->actionVerify, ui->actionProgram})
    connect(action, &QAction::triggered, this,
            &ProgrammerMain::updateDeviceOperations);

  for (auto action : {ui->actionHarware_settings, ui->actionDevice})
    connect(action, &QAction::triggered, this, [this, action]() {
      openSettingsWindow(ui->menuOptions->actions().indexOf(action));
    });
  // disabled for now
  ui->actionJTAG_Settings->setVisible(false);
  ui->actionOptions->setVisible(false);

  connect(m_guiIntegration, &ProgrammerGuiIntegration::progress, this,
          &ProgrammerMain::progressChanged);

  connect(m_guiIntegration, &ProgrammerGuiIntegration::autoDetect, this,
          &ProgrammerMain::autoDetect);

  ui->actionProgram->setDisabled(true);  // TODO temporary
  TclConsoleBuffer *buffer = new TclConsoleBuffer{};
  auto tclConsole = std::make_unique<FOEDAG::TclConsole>(
      GlobalSession->TclInterp()->getInterp(), buffer->getStream());
  TclConsoleWidget *console{nullptr};
  QWidget *w =
      FOEDAG::createConsole(GlobalSession->TclInterp()->getInterp(),
                            std::move(tclConsole), buffer, nullptr, &console);
  ui->groupBox->layout()->addWidget(w);
  console->addParser(new TclErrorParser{});
  console->addParser(new FileNameParser{});
}

ProgrammerMain::~ProgrammerMain() { delete ui; }

void ProgrammerMain::gui_start(bool showWP) { GetDeviceList(); }

bool ProgrammerMain::isRunning() const { return m_programmingDone == false; }

QString ProgrammerMain::cfgFile() const { return m_cfgFile; }

void ProgrammerMain::setCfgFile(const QString &cfg) { m_cfgFile = cfg; }

void ProgrammerMain::closeEvent(QCloseEvent *e) {
  if (isRunning()) {
    QMessageBox::warning(this, "Programming in progress",
                         "Please stop programming process.");
    e->ignore();
    return;
  }
  if (QMessageBox::question(
          this, "Exit Program?", tr("Are you sure you want to exit?\n"),
          QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes)
    e->accept();
  else
    e->ignore();
}

void ProgrammerMain::onCustomContextMenu(const QPoint &point) {
  QModelIndex index = ui->treeWidget->indexAt(point);
  if (index.isValid()) {
    auto item = ui->treeWidget->itemAt(point);
    if (!item) return;
    updateOperationActions(item);
    m_currentItem = item;
    auto menu = prepareMenu(item->parent() != nullptr);
    menu->exec(ui->treeWidget->viewport()->mapToGlobal(point));
    menu->deleteLater();
  }
}

void ProgrammerMain::autoDetect() {
  cleanDeviceList();

  auto deviceList = m_guiIntegration->devices();

  for (auto &[cab, devs] : deviceList.values()) {
    for (const auto &dev : devs) {
      DeviceInfo *deviceInfo = new DeviceInfo;
      deviceInfo->dev = dev;
      deviceInfo->cable = cab;
      if (dev.flashSize > 0) {
        auto flash = new DeviceInfo;
        flash->isFlash = true;
        flash->dev = dev;
        flash->cable = cab;
        deviceInfo->flash = flash;
      }
      m_deviceSettings.push_back(deviceInfo);
    }
  }
  updateTable();
  ui->groupBoxDeviceTree->setTitle(
      QString{"Device Tree: %1 device(s) found"}.arg(
          QString::number(m_deviceSettings.count())));
}

void ProgrammerMain::startPressed() {
  ui->toolBar->removeAction(ui->actionStart);
  ui->toolBar->insertAction(m_progressAction, ui->actionStop);
  ui->actionDetect->setEnabled(false);

  if (VerifyDevices()) {
    start();
  } else {
    QMessageBox::critical(this, "Programming error",
                          "Please select file (s) and define Operations");
  }

  ui->actionDetect->setEnabled(true);
  ui->toolBar->removeAction(ui->actionStop);
  ui->toolBar->insertAction(m_progressAction, ui->actionStart);
}

void ProgrammerMain::stopPressed() { stop = true; }

void ProgrammerMain::addFile() {
  auto option{QFileDialog::DontUseNativeDialog};
  const QString fileName = QFileDialog::getOpenFileName(
      this, tr("Select File"), "", "Bitstream file (*)", nullptr, option);

  if (m_currentItem) SetFile(m_items.value(m_currentItem, nullptr), fileName);
}

void ProgrammerMain::reset() {
  if (m_currentItem) SetFile(m_items.value(m_currentItem), {});
}

void ProgrammerMain::showToolTip() {
  QToolTip::showText(QCursor::pos(), "Hello, world!", this);
}

void ProgrammerMain::updateProgressSlot(QProgressBar *progressBar, int value) {
  progressBar->setValue(value);
}

void ProgrammerMain::updateDeviceOperations(bool ok) {
  if (m_currentItem) {
    QStringList operation{};
    if (ui->actionProgram->isChecked()) operation.append("Program");
    if (ui->actionErase->isChecked()) operation.append("Erase");
    if (ui->actionVerify->isChecked()) operation.append("Verify");
    if (ui->actionBlankcheck->isChecked()) operation.append("Blankcheck");
    auto deviceInfo = m_items.value(m_currentItem);
    deviceInfo->options.operations = operation;
    updateRow(m_currentItem, deviceInfo);
  }
}

void ProgrammerMain::progressChanged(const std::string &progress) {
  const auto &[currentCable, currentDevice] = m_guiIntegration->CurrentDevice();
  for (auto dev : qAsConst(m_deviceSettings)) {
    if (dev->cable == currentCable && dev->dev == currentDevice) {
      auto device = m_guiIntegration->IsFlash() ? dev->flash : dev;
      if (device->options.progress) device->options.progress(progress);
      SetFile(device, QString::fromStdString(m_guiIntegration->File(
                          currentDevice, m_guiIntegration->IsFlash())));
      int progressInt = QString::fromStdString(progress).toInt();
      setStatus(device,
                (progressInt == 100) ? Status::Done : Status::InProgress);
      break;
    }
  }
}

void ProgrammerMain::openSettingsWindow(int index) {
  ProgrammerSettingsWidget w{m_settings, this};
  w.setWindowTitle("Settings");
  w.openTab(index);
  w.exec();
}

bool ProgrammerMain::EvalCommand(const QString &cmd) {
  return GlobalSession->CmdStack()->push_and_exec(
      new Command{cmd.toStdString()});
}

bool ProgrammerMain::EvalCommand(const std::string &cmd) {
  return GlobalSession->CmdStack()->push_and_exec(new Command{cmd});
}

void ProgrammerMain::SetFile(DeviceInfo *device, const QString &file) {
  if (!device) return;
  device->options.file = file;
  if (!device->isFlash) {  // device
    device->options.operations =
        file.isEmpty() ? QStringList{} : QStringList{{"Configure"}};
  } else {
    device->options.operations =
        file.isEmpty() ? QStringList{} : QStringList{{"Program"}};
  }
  updateRow(m_items.key(device, nullptr), device);
}

QString ProgrammerMain::ToString(const QString &str) {
  return str.isEmpty() ? NONE_STR : str;
}

QString ProgrammerMain::ToString(const QStringList &strList,
                                 const QString &sep) {
  return strList.isEmpty() ? NONE_STR : strList.join(sep);
}

void ProgrammerMain::GetDeviceList() {
  cleanDeviceList();
  EvalCommand(std::string{"programmer list_cable"});
  EvalCommand(std::string{"programmer list_device"});
}

void ProgrammerMain::updateTable() {
  ui->treeWidget->clear();
  m_mainProgress.clear();
  m_items.clear();
  int counter{0};
  for (auto deviceInfo : qAsConst(m_deviceSettings)) {
    auto top = new QTreeWidgetItem{BuildDeviceRow(*deviceInfo, ++counter)};
    top->setIcon(0, QIcon{":/images/electronics-chip.png"});
    m_items.insert(top, deviceInfo);
    ui->treeWidget->addTopLevelItem(top);
    auto progress = new QProgressBar{this};
    progress->setValue(0);
    m_mainProgress.AddProgressBar(progress);
    deviceInfo->options.progress = [this, progress](const std::string &val) {
      emit updateProgress(progress, QString::fromStdString(val).toDouble());
    };
    ui->treeWidget->setItemWidget(top, PROGRESS_COL, progress);
    if (deviceInfo->flash) {
      auto flash = new QTreeWidgetItem{BuildFlashRow(*deviceInfo->flash)};
      m_items.insert(flash, deviceInfo->flash);
      top->addChild(flash);
      progress = new QProgressBar{this};
      progress->setValue(0);
      m_mainProgress.AddProgressBar(progress);
      deviceInfo->flash->options.progress = [this,
                                             progress](const std::string &val) {
        emit updateProgress(progress, QString::fromStdString(val).toDouble());
      };
      ui->treeWidget->setItemWidget(flash, PROGRESS_COL, progress);
    }
  }
  ui->treeWidget->expandAll();
}

void ProgrammerMain::updateOperationActions(QTreeWidgetItem *item) {
  // Program, Verify Erase & Blankcheck
  DeviceInfo *deviceIndo = m_items.value(item);
  QStringList operations = deviceIndo->options.operations;
  ui->actionProgram->setChecked(operations.contains("Program"));
  ui->actionErase->setChecked(operations.contains("Erase"));
  ui->actionVerify->setChecked(operations.contains("Verify"));
  ui->actionBlankcheck->setChecked(operations.contains("Blankcheck"));
}

void ProgrammerMain::updateRow(QTreeWidgetItem *item, DeviceInfo *deviceInfo) {
  if (item && deviceInfo) {
    item->setText(FILE_COL, ToString(deviceInfo->options.file));
    item->setText(OPERATIONS_COL,
                  ToString(deviceInfo->options.operations, ", "));
  }
}

int ProgrammerMain::itemIndex(QTreeWidgetItem *item) const {
  auto parent = item->parent() == nullptr ? item : item->parent();
  return ui->treeWidget->indexOfTopLevelItem(parent);
}

QMenu *ProgrammerMain::prepareMenu(bool flash) {
  QMenu *contextMenu = new QMenu;
  contextMenu->addAction(ui->actionAdd_File);
  contextMenu->addAction(ui->actionReset);
  //  if (flash) {
  //    contextMenu->addSeparator();
  // TODO not all operations supported yet
  //    contextMenu->addAction(ui->actionProgram);
  //    contextMenu->addAction(ui->actionVerify);
  //    contextMenu->addAction(ui->actionErase);
  //    contextMenu->addAction(ui->actionBlankcheck);
  //  }
  return contextMenu;
}

void ProgrammerMain::cleanupStatusAndProgress() {
  for (auto dev : qAsConst(m_deviceTmp)) {
    if (dev->options.progress) dev->options.progress({});
    setStatus(dev, Pending);
  }
}

void ProgrammerMain::cleanDeviceList() {
  qDeleteAll(m_deviceSettings);
  m_deviceSettings.clear();
  m_items.clear();
  m_deviceTmp.clear();
}

QStringList ProgrammerMain::BuildDeviceRow(const DeviceInfo &dev, int counter) {
  return {QString{"%1: %2"}.arg(QString::number(counter),
                                ToString(QString::fromStdString(dev.dev.name))),
          NONE_STR, NONE_STR, NONE_STR};
}

QStringList ProgrammerMain::BuildFlashRow(const DeviceInfo &dev) {
  return {QString{"Flash"}, NONE_STR, NONE_STR, NONE_STR};
}

bool ProgrammerMain::VerifyDevices() {
  auto fileEmpty = [](DeviceInfo *dev) {
    return dev &&
           (dev->options.file.isEmpty() || dev->options.operations.isEmpty());
  };
  if (std::any_of(m_deviceSettings.begin(), m_deviceSettings.end(),
                  [fileEmpty](DeviceInfo *dev) {
                    return fileEmpty(dev) || fileEmpty(dev->flash);
                  }))
    return false;
  return true;
}

void ProgrammerMain::start() {
  m_programmingDone = false;
  stop = false;
  if (m_deviceTmp.isEmpty()) {
    for (auto d : qAsConst(m_deviceSettings)) {
      m_deviceTmp.push_back(d);
      if (d->flash) m_deviceTmp.push_back(d->flash);
    }
  }
  cleanupStatusAndProgress();
  while (!m_deviceTmp.isEmpty()) {
    if (stop) break;
    auto dev = m_deviceTmp.first();
    bool result{false};
    if (!dev->isFlash) {  // device
      result = EvalCommand(QString{"programmer fpga_config -c %1 -d %2 %3"}.arg(
          QString::fromStdString(dev->cable.name),
          QString::number(dev->dev.index), dev->options.file));
    } else {  // flash
      auto operations = dev->options.operations.join(",").toLower();
      result = EvalCommand(QString{"programmer flash -c %1 -d %2 -o %3 %4"}.arg(
          QString::fromStdString(dev->cable.name),
          QString::number(dev->dev.index), operations, dev->options.file));
    }
    if (!result) break;
    m_deviceTmp.removeFirst();
  }
  m_programmingDone = true;
}

void ProgrammerMain::setStatus(DeviceInfo *deviceInfo, Status status) {
  auto item = m_items.key(deviceInfo);
  if (item) {
    item->setText(STATUS_COL, ToString(status));
  }
}

QString ProgrammerMain::ToString(Status status) {
  switch (status) {
    case None:
      return NONE_STR;
    case Pending:
      return "Pending";
    case InProgress:
      return "In progress";
    case Done:
      return "Done";
  }
  return NONE_STR;
}

}  // namespace FOEDAG
