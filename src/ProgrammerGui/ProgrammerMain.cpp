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
#include "ProgrammerSettingsWidget.h"
#include "qdebug.h"
#include "ui_ProgrammerMain.h"

inline void InitResources() {
  Q_INIT_RESOURCE(res);
  Q_INIT_RESOURCE(main_window_resource);
}

extern FOEDAG::Session *GlobalSession;

namespace FOEDAG {

static const char *NONE_STR{"-"};

bool StartThread(const std::function<bool(void)> &fn) {
  bool result = true;
  QEventLoop eventLoop{};
  auto m_thread = std::thread([&]() {
    result = fn();
    eventLoop.quit();
  });
  eventLoop.exec();
  m_thread.join();
  return result;
}

ProgrammerMain::ProgrammerMain(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ProgrammerMain),
      m_settings("programmer_settings", QSettings::IniFormat) {
  InitResources();
  ui->setupUi(this);

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
  connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
          SLOT(onCustomContextMenu(QPoint)));
  connect(ui->actionExit, &QAction::triggered, this, &ProgrammerMain::close);
  connect(ui->actionDetect, &QAction::triggered, this,
          &ProgrammerMain::autoDetect);
  connect(ui->actionStart, &QAction::triggered, this,
          &ProgrammerMain::startPressed);
  connect(ui->actionStop, &QAction::triggered, this,
          &ProgrammerMain::stopPressed);
  connect(ui->actionAdd_File, &QAction::triggered, this,
          &ProgrammerMain::addFile);
  connect(ui->actionReset, &QAction::triggered, this, &ProgrammerMain::reset);
  ui->toolBar->removeAction(ui->actionStop);
  autoDetect();
  QIcon image{":/images/help-circle.png"};
  auto btn = new QPushButton{image, {}};
  btn->setFlat(true);
  connect(btn, &QPushButton::clicked, this, &ProgrammerMain::showToolTip);
  menuBar()->setCornerWidget(btn);

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

QString ProgrammerMain::cfgFile() const { return m_cfgFile; }

void ProgrammerMain::setCfgFile(const QString &cfg) { m_cfgFile = cfg; }

void ProgrammerMain::closeEvent(QCloseEvent *e) {
  if (!m_programmingDone) {
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
  std::vector<FoedagDevice> devices{};
  auto deviceList = m_backend.ListDevicesAPI(devices);
  if (deviceList.first) {
    qDeleteAll(m_deviceSettings);
    m_deviceSettings.clear();
    m_items.clear();
    m_deviceTmp.clear();
    for (const auto &device : devices) {
      DeviceSettings *ds = new DeviceSettings;
      ds->device = device;
      ds->flash = new DeviceSettings;
      ds->flash->device = device;
      ds->flash->isFlash = true;
      m_deviceSettings.push_back(ds);
    }
    updateTable();
  }
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

  if (m_currentItem) {
    auto ds = m_items.value(m_currentItem);
    ds->devOptions.file = fileName;
    if (ds->flash) {  // device
      ds->devOptions.operations = QStringList{{"Configure"}};
    }
    updateRow(m_currentItem);
  }
}

void ProgrammerMain::reset() {
  if (m_currentItem) {
    auto ds = m_items.value(m_currentItem);
    ds->devOptions.file.clear();
    ds->devOptions.operations.clear();
    updateRow(m_currentItem);
  }
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
    auto ds = m_items.value(m_currentItem);
    ds->devOptions.operations = operation;
    updateRow(m_currentItem);
  }
}

void ProgrammerMain::openSettingsWindow(int index) {
  ProgrammerSettingsWidget w{m_settings, this};
  w.setWindowTitle("Settings");
  w.openTab(index);
  w.exec();
}

void ProgrammerMain::updateTable() {
  ui->treeWidget->clear();
  m_mainProgress.clear();
  m_items.clear();
  int counter{0};
  for (auto ds : m_deviceSettings) {
    auto top = new QTreeWidgetItem{BuildDeviceRow(*ds, ++counter)};
    top->setIcon(0, QIcon{":/images/electronics-chip.png"});
    m_items.insert(top, ds);
    ui->treeWidget->addTopLevelItem(top);
    auto progress = new QProgressBar{this};
    progress->setValue(0);
    m_mainProgress.AddProgressBar(progress);
    ds->devOptions.progress = [this, progress](const std::string &val) {
      emit updateProgress(progress, QString::fromStdString(val).toDouble());
    };
    ui->treeWidget->setItemWidget(top, PROGRESS_COL, progress);
    auto flash = new QTreeWidgetItem{BuildFlashRow(*ds->flash)};
    m_items.insert(flash, ds->flash);
    top->addChild(flash);
    progress = new QProgressBar{this};
    progress->setValue(0);
    m_mainProgress.AddProgressBar(progress);
    ds->flash->devOptions.progress = [this, progress](const std::string &val) {
      emit updateProgress(progress, QString::fromStdString(val).toDouble());
    };
    ui->treeWidget->setItemWidget(flash, PROGRESS_COL, progress);
  }
  ui->treeWidget->expandAll();
}

void ProgrammerMain::updateOperationActions(QTreeWidgetItem *item) {
  // Program, Verify Erase & Blankcheck
  DeviceSettings *ds = m_items.value(item);
  QStringList operations = ds->devOptions.operations;
  ui->actionProgram->setChecked(operations.contains("Program"));
  ui->actionErase->setChecked(operations.contains("Erase"));
  ui->actionVerify->setChecked(operations.contains("Verify"));
  ui->actionBlankcheck->setChecked(operations.contains("Blankcheck"));
}

void ProgrammerMain::updateRow(QTreeWidgetItem *item) {
  auto ds = m_items.value(item);
  item->setText(FILE_COL,
                ds->devOptions.file.isEmpty() ? NONE_STR : ds->devOptions.file);
  item->setText(OPERATIONS_COL, ds->devOptions.operations.isEmpty()
                                    ? NONE_STR
                                    : ds->devOptions.operations.join(", "));
}

int ProgrammerMain::itemIndex(QTreeWidgetItem *item) const {
  auto parent = item->parent() == nullptr ? item : item->parent();
  return ui->treeWidget->indexOfTopLevelItem(parent);
}

QMenu *ProgrammerMain::prepareMenu(bool flash) {
  QMenu *contextMenu = new QMenu;
  contextMenu->addAction(ui->actionAdd_File);
  contextMenu->addAction(ui->actionReset);
  if (flash) {
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionProgram);
    contextMenu->addAction(ui->actionVerify);
    contextMenu->addAction(ui->actionErase);
    contextMenu->addAction(ui->actionBlankcheck);
  }
  return contextMenu;
}

void ProgrammerMain::cleanup() {
  for (auto dev : m_deviceTmp) {
    dev->devOptions.progress({});
    setStatus(dev, Pending);
  }
}

QStringList ProgrammerMain::BuildDeviceRow(const DeviceSettings &dev,
                                           int counter) {
  return {QString{"%1: %2"}.arg(
              QString::number(counter),
              dev.device.name.isEmpty() ? NONE_STR : dev.device.name),
          NONE_STR, NONE_STR, NONE_STR};
}

QStringList ProgrammerMain::BuildFlashRow(const DeviceSettings &dev) {
  return {QString{"Flash"}, NONE_STR, NONE_STR, NONE_STR};
}

bool ProgrammerMain::VerifyDevices() {
  /* temporary turned off
  for (const auto &dev : m_deviceSettings) {
    if (dev.devOptions.file.isEmpty() || dev.devOptions.operations.isEmpty())
      return false;
    if (dev.flashOptions.file.isEmpty() ||
        dev.flashOptions.operations.isEmpty())
      return false;
  }
  */
  return true;
}

void ProgrammerMain::start() {
  m_programmingDone = false;
  std::ostream *outStream = nullptr;
  stop = false;
  auto outputCallback = [this](const QString &msg) { emit appendOutput(msg); };
  if (m_deviceTmp.isEmpty()) {
    for (auto d : m_deviceSettings) {
      m_deviceTmp.push_back(d);
      m_deviceTmp.push_back(d->flash);
    }
  }
  cleanup();
  while (!m_deviceTmp.isEmpty()) {
    auto dev = m_deviceTmp.first();
    setStatus(dev, InProgress);
    auto result = StartThread([&]() {
      auto returnValue{0};
      if (!dev->isFlash) {
        returnValue = m_backend.ProgramFpgaAPI(
            dev->device, dev->devOptions.file, m_cfgFile, outStream,
            outputCallback, dev->devOptions.progress, &stop);
      } else {
        returnValue = m_backend.ProgramFlashAPI(
            dev->device, dev->devOptions.file, m_cfgFile, outStream,
            outputCallback, dev->devOptions.progress, &stop);
      }
      return m_backend.StatusAPI(dev->device) && (returnValue == 0);
    });
    if (!result) break;
    setStatus(dev, Done);
    m_deviceTmp.removeFirst();
  }
  m_programmingDone = true;
}

void ProgrammerMain::setStatus(DeviceSettings *ds, Status status) {
  auto item = m_items.key(ds);
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
  return {};
}

}  // namespace FOEDAG
