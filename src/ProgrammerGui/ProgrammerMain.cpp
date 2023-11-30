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

#include "Configuration/CFGCommon/CFGCommon.h"
#include "Console/FileNameParser.h"
#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleBuilder.h"
#include "Console/TclConsoleWidget.h"
#include "Console/TclErrorParser.h"
#include "MainWindow/Session.h"
#include "ProgrammerGuiIntegration.h"
#include "ProgrammerSettingsWidget.h"
#include "Utils/QtUtils.h"
#include "ui_ProgrammerMain.h"

inline void InitResources() {
  Q_INIT_RESOURCE(res);
  Q_INIT_RESOURCE(main_window_resource);
}

extern FOEDAG::Session *GlobalSession;

namespace FOEDAG {

static const char *NONE_STR{"-"};
static const char *Configure{"Configure"};
static const char *ProgramOtp{"Program OTP"};
static const char *Program{"Program"};
static const char *Erase{"Erase"};
static const char *Verify{"Verify"};
static const char *Blankcheck{"Blankcheck"};

ProgrammerMain::ProgrammerMain(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ProgrammerMain),
      m_settings("programmer_settings", QSettings::IniFormat),
      m_guiIntegration(new ProgrammerGuiIntegration{this}) {
  InitResources();
  ui->setupUi(this);
  qRegisterMetaType<DeviceEntity>("DeviceEntity");

  Gui::SetGuiInterface(m_guiIntegration);

  m_hardware = new QComboBox;
  m_hardware->setToolTip("Select the hardware cable type");
  loadFromSettigns();
  m_hardware->setFixedWidth(120);
  connect(m_hardware, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &ProgrammerMain::updateTable);

  QLabel *label1 = new QLabel("Hardware:");
  label1->setBuddy(m_hardware);

  m_iface = new QComboBox;
  m_iface->setToolTip("Select the programming interface protocol");
  m_iface->addItem("JTAG");
  m_iface->setFixedWidth(120);

  QLabel *label2 = new QLabel("Interface:");
  label2->setBuddy(m_iface);

  m_mainProgress.progressBar()->setAlignment(Qt::AlignHCenter);

  ui->toolBar->insertWidget(ui->actionDetect, label1);
  ui->toolBar->insertWidget(ui->actionDetect, m_hardware);
  ui->toolBar->insertWidget(ui->actionDetect, label2);
  ui->toolBar->insertWidget(ui->actionDetect, m_iface);
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
  connect(ui->treeWidget, &QTreeWidget::itemChanged, this,
          &ProgrammerMain::itemHasChanged);
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
          [](QProgressBar *progressBar, int value) {
            progressBar->setValue(value);
          });

  for (QAction *action :
       {ui->actionBlankcheck, ui->actionErase, ui->actionVerify,
        ui->actionProgram, ui->actionProgram_OTP, ui->actionConfigure})
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
  connect(m_guiIntegration, &ProgrammerGuiIntegration::programStarted, this,
          &ProgrammerMain::programStarted);
  connect(m_guiIntegration, &ProgrammerGuiIntegration::status, this,
          &ProgrammerMain::updateStatus);

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
  auto compiler = GlobalSession->GetCompiler();
  compiler->SetInterpreter(GlobalSession->TclInterp());
  compiler->SetOutStream(&buffer->getStream());
  compiler->SetErrStream(&console->getErrorBuffer()->getStream());
  setWindowTitle(ProgrammerTitle());

  ui->actionDetect->setToolTip(
      "Auto detect connected devices and populate in device tree");
  ui->actionStart->setToolTip("Start programming");
  ui->actionStop->setToolTip("Stop programming");
  m_mainProgress.progressBar()->setToolTip("Overall programming progress bar");
  ui->treeWidget->setToolTip(
      "Select row. Right click to add bitstream file and select operation.");
  QActionGroup *group = new QActionGroup{this};
  group->addAction(ui->actionConfigure);
  group->addAction(ui->actionProgram_OTP);

  m_defaultPalette = m_mainProgress.progressBar()->palette();
  m_failPalette = m_passPalette = m_defaultPalette;
  m_failPalette.setColor(QPalette::ColorRole::Highlight,
                         StatusColor(Status::Failed));
  m_passPalette.setColor(QPalette::ColorRole::Highlight,
                         StatusColor(Status::Done));
}

ProgrammerMain::~ProgrammerMain() { delete ui; }

void ProgrammerMain::gui_start(bool showWP) { GetDeviceList(); }

bool ProgrammerMain::isRunning() const { return m_programmingDone == false; }

void ProgrammerMain::closeEvent(QCloseEvent *e) {
  if (isRunning()) {
    if (InProgressMessageBoxAccepted(this))
      e->accept();
    else
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
  if (!m_programmingDone) return;
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

  const QSignalBlocker signalBlocker{m_hardware};
  m_hardware->clear();

  for (auto &[cab, devs] : deviceList.values()) {
    m_frequency[cab] = Frequency;
    for (const auto &dev : devs) {
      DeviceInfo *deviceInfo = new DeviceInfo;
      deviceInfo->dev = dev;
      deviceInfo->cable = cab;
      if (dev.hasFlash() > 0) {
        auto flash = new DeviceInfo;
        flash->isFlash = true;
        flash->dev = dev;
        flash->cable = cab;
        deviceInfo->flash = flash;
      }
      m_deviceSettings.push_back(deviceInfo);
      m_hardware->addItem(cab.name(),
                          QVariant::fromValue<FOEDAG::ProgrammerCable>(cab));
    }
  }
  updateTable();
  ui->groupBoxDeviceTree->setTitle(
      QString{"Device Tree: %1 device(s) found"}.arg(
          QString::number(m_deviceSettings.count())));
}

void ProgrammerMain::itemHasChanged(QTreeWidgetItem *item, int column) {
  if (column == TITLE_COL) {
    if (item->checkState(column) == Qt::Checked) {
      m_mainProgress.AddProgressBar(dynamic_cast<QProgressBar *>(
          ui->treeWidget->itemWidget(item, PROGRESS_COL)));
    } else {
      m_mainProgress.RemoveProgressBar(dynamic_cast<QProgressBar *>(
          ui->treeWidget->itemWidget(item, PROGRESS_COL)));
    }
  }
}

void ProgrammerMain::updateStatus(const DeviceEntity &entity, int status) {
  for (auto devInfo : qAsConst(m_deviceSettings)) {
    if (devInfo->cable == entity.cable && devInfo->dev == entity.device) {
      auto device = (entity.type == Type::Flash) ? devInfo->flash : devInfo;
      setStatus(device, (status == 0) ? Done : Failed);
      break;
    }
  }
}

void ProgrammerMain::startPressed() {
  m_mainProgress.progressBar()->setPalette(m_defaultPalette);
  ui->toolBar->removeAction(ui->actionStart);
  ui->toolBar->insertAction(m_progressAction, ui->actionStop);
  ui->actionDetect->setEnabled(false);
  ui->actionStop->setEnabled(true);

  if (VerifyDevices()) start();

  ui->actionDetect->setEnabled(true);
  ui->toolBar->removeAction(ui->actionStop);
  ui->toolBar->insertAction(m_progressAction, ui->actionStart);
}

void ProgrammerMain::stopPressed() {
  ui->actionStop->setEnabled(false);
  m_stop = true;
  m_guiIntegration->StopLastProcess();
  GlobalSession->GetCompiler()->ErrorMessage("Interrupted by user");
}

void ProgrammerMain::addFile() {
  auto option{QFileDialog::DontUseNativeDialog};
  const QString fileName = QFileDialog::getOpenFileName(
      this, tr("Select File"), "", "Bitstream file (*)", nullptr, option);

  if (m_currentItem)
    SetFile(m_items.value(m_currentItem, nullptr), fileName, false);
}

void ProgrammerMain::reset() {
  if (m_currentItem) {
    auto deviceInfo = m_items.value(m_currentItem);
    SetFile(deviceInfo, {}, false);
    m_currentItem->setCheckState(TITLE_COL, DefaultCheckState);
    if (deviceInfo->options.progress) deviceInfo->options.progress({});
    setStatus(deviceInfo, Status::None);
  }
}

void ProgrammerMain::showToolTip() {
  QToolTip::showText(QCursor::pos(), "Hello, world!", this);
}

void ProgrammerMain::updateDeviceOperations(bool ok) {
  if (m_currentItem) {
    QStringList operation{};
    auto deviceInfo = m_items.value(m_currentItem);
    if (deviceInfo->isFlash) {
      if (ui->actionProgram->isChecked()) operation.append(Program);
      if (ui->actionErase->isChecked()) operation.append(Erase);
      if (ui->actionVerify->isChecked()) operation.append(Verify);
      if (ui->actionBlankcheck->isChecked()) operation.append(Blankcheck);
    } else {
      if (ui->actionConfigure->isChecked()) operation.append(Configure);
      if (ui->actionProgram_OTP->isChecked()) operation.append(ProgramOtp);
    }
    deviceInfo->options.operations = operation;
    updateRow(m_currentItem, deviceInfo);
  }
}

void ProgrammerMain::progressChanged(const DeviceEntity &entity,
                                     const std::string &progress) {
  for (auto devInfo : qAsConst(m_deviceSettings)) {
    if (devInfo->cable == entity.cable && devInfo->dev == entity.device) {
      auto device = (entity.type == Type::Flash) ? devInfo->flash : devInfo;
      if (device->options.progress) device->options.progress(progress);
      break;
    }
  }
}

void ProgrammerMain::programStarted(const DeviceEntity &entity) {
  for (auto devInfo : qAsConst(m_deviceSettings)) {
    if (devInfo->cable == entity.cable && devInfo->dev == entity.device) {
      auto device = (entity.type == Type::Flash) ? devInfo->flash : devInfo;
      SetFile(device,
              QString::fromStdString(m_guiIntegration->File(
                  devInfo->dev, (entity.type == Type::Flash))),
              (entity.type == Type::Otp));
      setStatus(device, InProgress);
      break;
    }
  }
}

void ProgrammerMain::openSettingsWindow(int index) {
  ProgrammerSettings settings;
  settings.frequency = m_frequency;
  settings.devices = m_deviceSettings;
  ProgrammerSettingsWidget w{settings, m_settings, this};
  w.setWindowTitle("Settings");
  w.openTab(index);
  if (w.exec() == QDialog::Accepted) {
    loadFromSettigns();
  }
}

bool ProgrammerMain::EvalCommand(const QString &cmd) {
  return GlobalSession->CmdStack()->push_and_exec(
      new Command{cmd.toStdString()});
}

bool ProgrammerMain::EvalCommand(const std::string &cmd) {
  return GlobalSession->CmdStack()->push_and_exec(new Command{cmd});
}

void ProgrammerMain::SetFile(DeviceInfo *device, const QString &file,
                             bool otp) {
  if (!device) return;
  device->options.file = file;
  if (!device->isFlash) {  // device
    if (device->options.operations.isEmpty()) {
      device->options.operations =
          file.isEmpty()
              ? QStringList{}
              : (otp ? QStringList{ProgramOtp} : QStringList{Configure});
    } else {
      if (file.isEmpty()) device->options.operations.clear();
    }
  } else {
    device->options.operations =
        file.isEmpty() ? QStringList{} : QStringList{Program};
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

void ProgrammerMain::loadFromSettigns() {
  for (auto &[cable, freq] : m_frequency.values()) {
    auto settingKey = HardwareFrequencyKey().arg(cable.name());
    if (m_settings.contains(settingKey)) {
      m_frequency[cable] = m_settings.value(settingKey).toUInt();
    }
  }
}

bool ProgrammerMain::IsEnabled(DeviceInfo *deviceInfo) const {
  auto item = m_items.key(deviceInfo);
  return item ? item->checkState(TITLE_COL) == Qt::Checked : false;
}

QColor ProgrammerMain::StatusColor(Status status) {
  switch (status) {
    case None:
      return Qt::black;
    case Pending:
      return QColor{200, 200, 80};
    case InProgress:
      return QColor{100, 100, 255};
    case Done:
      return QColor{100, 255, 100};
    case Failed:
      return QColor{255, 100, 100};
  }
  return Qt::black;
}

bool ProgrammerMain::InProgressMessageBoxAccepted(QWidget *parent) {
  QMessageBox question{parent};
  question.setIcon(QMessageBox::Warning);
  question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  question.setWindowTitle("Programming in progress");
  question.setText(
      "Important Note: The FPGA configuration or flashing process is "
      "currently in progress. Closing the software now could result in "
      "incomplete configuration, the FPGA/flash may be left partially "
      "configured, leading to unpredictable behavior.");
  question.setButtonText(QMessageBox::Yes,
                         "Yes, close the software \n(Not recommended)");
  question.setButtonText(QMessageBox::No,
                         "No, I want to resume or complete \nthe programming "
                         "process (Recommended)");
  question.setDefaultButton(QMessageBox::No);
  return question.exec() == QMessageBox::Yes;
}

QPalette ProgrammerMain::StatusPalette(int status) const {
  return status == Status::Done
             ? m_passPalette
             : status == Status::Failed ? m_failPalette : m_defaultPalette;
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
  auto currentCable = m_hardware->currentData().value<ProgrammerCable>();
  for (auto deviceInfo : qAsConst(m_deviceSettings)) {
    if (!(currentCable == deviceInfo->cable)) continue;
    auto top = new QTreeWidgetItem{BuildDeviceRow(*deviceInfo, ++counter)};
    top->setIcon(TITLE_COL, QIcon{":/images/electronics-chip.png"});
    m_items.insert(top, deviceInfo);
    ui->treeWidget->addTopLevelItem(top);
    auto progress = new QProgressBar{this};
    progress->setValue(0);
    deviceInfo->options.progress = [this, progress](const std::string &val) {
      emit updateProgress(progress, QString::fromStdString(val).toDouble());
    };
    ui->treeWidget->setItemWidget(top, PROGRESS_COL, progress);
    top->setCheckState(TITLE_COL, DefaultCheckState);
    if (deviceInfo->flash) {
      auto flash = new QTreeWidgetItem{BuildFlashRow(*deviceInfo->flash)};
      m_items.insert(flash, deviceInfo->flash);
      top->addChild(flash);
      progress = new QProgressBar{this};
      progress->setValue(0);
      deviceInfo->flash->options.progress = [this,
                                             progress](const std::string &val) {
        emit updateProgress(progress, QString::fromStdString(val).toDouble());
      };
      ui->treeWidget->setItemWidget(flash, PROGRESS_COL, progress);
      flash->setCheckState(TITLE_COL, DefaultCheckState);
    }
  }
  ui->treeWidget->expandAll();
}

void ProgrammerMain::updateOperationActions(QTreeWidgetItem *item) {
  // Program, Verify Erase & Blankcheck
  DeviceInfo *deviceIndo = m_items.value(item);
  QStringList operations = deviceIndo->options.operations;
  ui->actionProgram->setChecked(operations.contains(Program));
  ui->actionErase->setChecked(operations.contains(Erase));
  ui->actionVerify->setChecked(operations.contains(Verify));
  ui->actionBlankcheck->setChecked(operations.contains(Blankcheck));
  ui->actionProgram_OTP->setChecked(operations.contains(ProgramOtp));
  ui->actionConfigure->setChecked(operations.contains(Configure));
}

void ProgrammerMain::updateRow(QTreeWidgetItem *item, DeviceInfo *deviceInfo) {
  if (item && deviceInfo) {
    item->setText(FILE_COL, ToString(deviceInfo->options.file));
    item->setText(OPERATIONS_COL,
                  ToString(deviceInfo->options.operations, ", "));
    if (!deviceInfo->options.file.isEmpty())
      item->setCheckState(TITLE_COL, Qt::Checked);
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
  if (!flash) {
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionConfigure);
    contextMenu->addAction(ui->actionProgram_OTP);
  }
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
  auto clean = [this](DeviceInfo *deviceInfo) {
    if (deviceInfo->options.progress) deviceInfo->options.progress({});
    setStatus(deviceInfo, Pending);
  };
  for (auto dev : qAsConst(m_deviceSettings)) {
    if (IsEnabled(dev)) clean(dev);
    if (dev->flash && IsEnabled(dev->flash)) clean(dev->flash);
  }
}

void ProgrammerMain::cleanDeviceList() {
  qDeleteAll(m_deviceSettings);
  m_deviceSettings.clear();
  m_items.clear();
}

QStringList ProgrammerMain::BuildDeviceRow(const DeviceInfo &dev, int counter) {
  return {
      QString{"%1: %2"}.arg(QString::number(counter), ToString(dev.dev.name())),
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
                  [fileEmpty, this](DeviceInfo *dev) {
                    return (IsEnabled(dev) && fileEmpty(dev)) ||
                           (IsEnabled(dev->flash) && fileEmpty(dev->flash));
                  })) {
    QMessageBox::critical(this, "Programming error",
                          "Please select file (s) and define Operations");
    return false;
  }
  if (std::any_of(m_deviceSettings.begin(), m_deviceSettings.end(),
                  [this](DeviceInfo *dev) {
                    if (IsEnabled(dev)) {
                      return dev->options.operations.contains(ProgramOtp);
                    }
                    return false;
                  })) {
    auto answer = QMessageBox::question(
        this, "Confirm",
        "One-Time Programmable (OTP) Memory. The settings programmed into OTP "
        "memory are irreversible and cannot be modified. Please ensure all "
        "configurations are accurate and thoroughly tested before proceeding. "
        "Any errors or unintended settings may result in permanent "
        "functionality issues and require hardware replacement. Proceed with "
        "OTP programming only if you are certain about the programmed "
        "configurations.",
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Ok;
  }
  return true;
}

void ProgrammerMain::start() {
  m_programmingDone = false;
  m_stop = false;
  m_status = None;
  QVector<DeviceInfo *> runningDevices;
  for (auto d : qAsConst(m_deviceSettings)) {
    if (IsEnabled(d)) runningDevices.push_back(d);
    if (d->flash && IsEnabled(d->flash)) runningDevices.push_back(d->flash);
  }

  cleanupStatusAndProgress();
  while (!runningDevices.isEmpty()) {
    if (m_stop) break;
    auto dev = runningDevices.first();
    bool result{false};
    if (!dev->isFlash) {  // device
      if (dev->options.operations.contains(Configure)) {
        result =
            EvalCommand(QString{"programmer fpga_config -c %1 -d %2 %3"}.arg(
                dev->cable.name(), QString::number(dev->dev.index()),
                dev->options.file));
      } else if (dev->options.operations.contains(ProgramOtp)) {
        result = EvalCommand(QString{"programmer otp -c %1 -d %2 -y %3"}.arg(
            dev->cable.name(), QString::number(dev->dev.index()),
            dev->options.file));
      }
    } else {  // flash
      auto operations = dev->options.operations.join(",").toLower();
      result = EvalCommand(QString{"programmer flash -c %1 -d %2 -o %3 %4"}.arg(
          dev->cable.name(), QString::number(dev->dev.index()), operations,
          dev->options.file));
    }
    runningDevices.removeFirst();
    if (!result) break;
  }
  m_programmingDone = true;
  QtUtils::AppendToEventQueue([this]() {
    m_mainProgress.progressBar()->setPalette(StatusPalette(m_status));
  });
}

void ProgrammerMain::setStatus(DeviceInfo *deviceInfo, Status status) {
  auto item = m_items.key(deviceInfo);
  if (item) {
    m_status = status;
    item->setText(STATUS_COL, ToString(status));
    item->setForeground(STATUS_COL, QBrush{StatusColor(status)});
    ui->treeWidget->itemWidget(item, PROGRESS_COL)
        ->setPalette(StatusPalette(status));
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
    case Failed:
      return "Failed";
  }
  return NONE_STR;
}

}  // namespace FOEDAG
