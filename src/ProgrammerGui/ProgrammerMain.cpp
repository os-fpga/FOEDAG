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

#include "Compiler/WorkerThread.h"
#include "Utils/QtUtils.h"
#include "qdebug.h"
#include "ui_ProgrammerMain.h"

namespace FOEDAG {

QMenu *contextMenu = nullptr;

ProgrammerMain::ProgrammerMain(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ProgrammerMain) {
  ui->setupUi(this);

  contextMenu = new QMenu(this);
  contextMenu->addAction(ui->actionAdd_File);
  contextMenu->addAction(ui->actionReset);
  contextMenu->addSeparator();
  contextMenu->addAction(ui->actionProgram);
  contextMenu->addAction(ui->actionVerify);
  contextMenu->addAction(ui->actionErase);
  contextMenu->addAction(ui->actionBlankcheck);

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

  connect(this, &ProgrammerMain::appendOutput, this,
          [this](const QString &msg) {
            ui->plainTextEdit->insertPlainText(msg);
            ui->plainTextEdit->ensureCursorVisible();
          });
  connect(this, &ProgrammerMain::updateProgress, this,
          &ProgrammerMain::updateProgressSlot);
}

ProgrammerMain::~ProgrammerMain() { delete ui; }

void ProgrammerMain::closeEvent(QCloseEvent *e) {
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
    updateOperations(item);
    current = item;
    prepareMenu(item->parent() != nullptr);
    auto action =
        contextMenu->exec(ui->treeWidget->viewport()->mapToGlobal(point));
    if (action) {
      QStringList operation{};
      if (ui->actionProgram->isChecked()) operation.append("Program");
      if (ui->actionErase->isChecked()) operation.append("Erase");
      if (ui->actionVerify->isChecked()) operation.append("Verify");
      if (ui->actionBlankcheck->isChecked()) operation.append("Blankcheck");
      auto index = itemIndex(item);
      if (index >= 0 && index < m_deviceSettings.size()) {
        m_deviceSettings[index].flashOptions.operations = operation;
        updateTable();
      }
    }
  }
}

void ProgrammerMain::autoDetect() {
  std::vector<FoedagDevice> devices{};
  auto deviceList = m_backend.ListDevicesAPI(devices);
  if (deviceList.first) {
    m_deviceSettings.clear();
    for (const auto &device : devices) {
      DeviceSettings ds;
      ds.device = device;
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

  if (current) {
    auto index = itemIndex(current);
    if (index >= 0 && index < m_deviceSettings.size()) {
      if (current->parent()) {
        m_deviceSettings[index].flashOptions.file = fileName;
      } else {
        m_deviceSettings[index].devOptions.file = fileName;
        m_deviceSettings[index].devOptions.operations =
            QStringList{{"Configure"}};
      }
      updateTable();
    }
  }
}

void ProgrammerMain::reset() {
  if (current) {
    auto index = itemIndex(current);
    if (index >= 0 && index < m_deviceSettings.size()) {
      if (current->parent()) {
        m_deviceSettings[index].flashOptions.file.clear();
        m_deviceSettings[index].flashOptions.operations.clear();
      } else {
        m_deviceSettings[index].devOptions.file.clear();
        m_deviceSettings[index].devOptions.operations.clear();
      }
      updateTable();
    }
  }
}

void ProgrammerMain::showToolTip() {
  QToolTip::showText(QCursor::pos(), "Hello, world!", this);
}

void ProgrammerMain::updateProgressSlot(QProgressBar *progressBar, int value) {
  progressBar->setValue(value);
}

void ProgrammerMain::updateTable() {
  ui->treeWidget->clear();
  m_mainProgress.clear();
  m_deviceTmp.clear();
  int counter{0};
  for (auto &ds : m_deviceSettings) {
    auto top = new QTreeWidgetItem{BuildDeviceRow(ds, ++counter)};
    ui->treeWidget->addTopLevelItem(top);
    auto progress = new QProgressBar{this};
    m_mainProgress.AddProgressBar(progress);
    ds.devOptions.progress = [this, progress, ds](double val) {
      emit updateProgress(progress, val);
    };
    ui->treeWidget->setItemWidget(top, PROGRESS_COL, progress);
    auto flash = new QTreeWidgetItem{BuildFlashRow(ds)};
    top->addChild(flash);
    progress = new QProgressBar{this};
    m_mainProgress.AddProgressBar(progress);
    ds.flashOptions.progress = [this, progress, ds](double val) {
      emit updateProgress(progress, val);
    };
    ui->treeWidget->setItemWidget(flash, PROGRESS_COL, progress);
  }
  ui->treeWidget->expandAll();
}

void ProgrammerMain::updateOperations(QTreeWidgetItem *item) {
  // Program, Verify Erase & Blankcheck
  DeviceSettings ds = itemToDevice(item);
  QStringList operations = ds.flashOptions.operations;
  ui->actionProgram->setChecked(operations.contains("Program"));
  ui->actionErase->setChecked(operations.contains("Erase"));
  ui->actionVerify->setChecked(operations.contains("Verify"));
  ui->actionBlankcheck->setChecked(operations.contains("Blankcheck"));
}

DeviceSettings ProgrammerMain::itemToDevice(QTreeWidgetItem *item) const {
  int index = itemIndex(item);
  if (index >= 0 && index < m_deviceSettings.size())
    return m_deviceSettings.at(index);
  return {};
}

int ProgrammerMain::itemIndex(QTreeWidgetItem *item) const {
  auto parent = item->parent() == nullptr ? item : item->parent();
  return ui->treeWidget->indexOfTopLevelItem(parent);
}

void ProgrammerMain::prepareMenu(bool flash) {
  contextMenu->clear();
  contextMenu->addAction(ui->actionAdd_File);
  contextMenu->addAction(ui->actionReset);
  if (flash) {
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionProgram);
    contextMenu->addAction(ui->actionVerify);
    contextMenu->addAction(ui->actionErase);
    contextMenu->addAction(ui->actionBlankcheck);
  }
}

void ProgrammerMain::cleanup() {
  for (auto &dev : m_deviceTmp) {
    dev.flashOptions.progress(0);
    dev.devOptions.progress(0);
  }
}

QStringList ProgrammerMain::BuildDeviceRow(const DeviceSettings &dev,
                                           int counter) {
  return {
      QString{"%1: %2"}.arg(QString::number(counter),
                            dev.device.name.isEmpty() ? "-" : dev.device.name),
      dev.devOptions.file.isEmpty() ? "-" : dev.devOptions.file,
      dev.devOptions.operations.isEmpty()
          ? "-"
          : dev.devOptions.operations.join(", "),
      "-"};
}

QStringList ProgrammerMain::BuildFlashRow(const DeviceSettings &dev) {
  return {QString{"Flash"},
          dev.flashOptions.file.isEmpty() ? "-" : dev.flashOptions.file,
          dev.flashOptions.operations.isEmpty()
              ? "-"
              : dev.flashOptions.operations.join(", "),
          "-"};
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
  WorkerThread thread{{}, Compiler::Action::NoAction, nullptr};
  std::ostream *outStream = &std::cout;
  stop = false;
  auto outputCallback = [this](const QString &msg) { emit appendOutput(msg); };
  if (m_deviceTmp.isEmpty()) {
    m_deviceTmp = m_deviceSettings;
  }
  cleanup();
  while (!m_deviceTmp.isEmpty()) {
    auto dev = m_deviceTmp.first();
    auto ProgramFpga = [this, outputCallback](
                           const FoedagDevice &device, const QString &bitfile,
                           const QString &cfgfile, std::ostream *outStream,
                           OutputCallback outputMsg, ProgressCallback callback,
                           std::atomic<bool> *stop) -> bool {
      auto returnValue = m_backend.ProgramFpgaAPI(
          device, bitfile, cfgfile, outStream, outputCallback, callback, stop);
      return m_backend.StatusAPI(device) && (returnValue == 0);
    };
    auto result = thread.Start(ProgramFpga, dev.device, dev.devOptions.file,
                               QString{}, outStream, OutputCallback{},
                               dev.devOptions.progress, &stop);
    if (!result) break;
    auto ProgramFlash = [this](
                            const FoedagDevice &device, const QString &bitfile,
                            const QString &cfgfile, std::ostream *outStream,
                            OutputCallback outputMsg, ProgressCallback callback,
                            std::atomic<bool> *stop) -> bool {
      auto returnValue = m_backend.ProgramFlashAPI(
          device, bitfile, cfgfile, outStream, outputMsg, callback, stop);
      return m_backend.StatusAPI(device) && (returnValue == 0);
    };
    result = thread.Start(ProgramFlash, dev.device, dev.flashOptions.file,
                          QString{}, outStream, outputCallback,
                          dev.flashOptions.progress, &stop);
    if (!result) break;
    m_deviceTmp.removeFirst();
  }
}

}  // namespace FOEDAG
