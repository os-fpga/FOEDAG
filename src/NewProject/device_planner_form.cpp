#include "device_planner_form.h"

#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <filesystem>

#include "CustomLayout.h"
#include "ProjectManager/config.h"
#include "ProjectManager/project_manager.h"
#include "Utils/FileUtils.h"
#include "ui_device_planner_form.h"

using namespace FOEDAG;

devicePlannerForm::devicePlannerForm(const std::filesystem::path &deviceFile,
                                     QWidget *parent)
    : QWidget(parent), ui(new Ui::devicePlannerForm), m_deviceFile(deviceFile) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Select Target Device"));
  ui->m_labelDetail->setText(
      tr("Select the series and device you want to target for compilation."));

  m_tableView = new QTableView(this);

  // Set properties
  m_tableView->verticalHeader()->hide();
  m_tableView->verticalHeader()->setDefaultSectionSize(30);
  m_tableView->horizontalHeader()->setMinimumHeight(30);
  // Last column adaptive width
  m_tableView->horizontalHeader()->setStretchLastSection(true);
  m_tableView->setEditTriggers(QTableView::NoEditTriggers);
  m_tableView->setSelectionBehavior(QTableView::SelectRows);
  // Single line selection
  m_tableView->setSelectionMode(QTableView::SingleSelection);
  // Color separation between lines
  m_tableView->setAlternatingRowColors(true);
  m_tableView->setStyleSheet(
      "QTableView {border: 1px solid rgb(230,230,230);}\
       QTableView::item:selected{color:black;background:rgb(177,220,255);}");
  m_tableView->setColumnWidth(0, 80);

  m_model = new QStandardItemModel();
  m_selectmodel = new QItemSelectionModel(m_model);
  connect(m_selectmodel, &QItemSelectionModel::selectionChanged, this,
          &devicePlannerForm::updateEditDeviceButtons);
  connect(ui->pushButtonEdit, &QPushButton::clicked, this,
          &devicePlannerForm::editDevice);
  connect(ui->pushButtonRemove, &QPushButton::clicked, this,
          &devicePlannerForm::removeDevice);
  connect(ui->pushButtonCreate, &QPushButton::clicked, this,
          &devicePlannerForm::createDevice);

  m_tableView->horizontalHeader()->setMinimumHeight(30);

  m_tableView->setModel(m_model);
  m_tableView->setSelectionModel(m_selectmodel);

  QVBoxLayout *vbox = new QVBoxLayout(ui->m_frame);
  vbox->addWidget(m_tableView);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  ui->m_frame->setLayout(vbox);

  connect(ui->m_comboBoxFamily, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onFamilytextChanged);
  connect(ui->m_comboBoxSeries, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onSeriestextChanged);
  connect(ui->m_comboBoxPackage, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onPackagetextChanged);
  init({});
  updateEditDeviceButtons();
}

devicePlannerForm::~devicePlannerForm() { delete ui; }

void devicePlannerForm::CreateDevice_TclTest() {
  ui->pushButtonCreate->click();
}

void devicePlannerForm::EditDevice_TclTest() { ui->pushButtonEdit->click(); }

QString devicePlannerForm::selectedDeviceName() const {
  if (m_selectmodel->hasSelection() &&
      !m_selectmodel->selectedRows(0).isEmpty()) {
    auto index = m_selectmodel->selectedRows(0).first();
    return m_model->data(index).toString();
  }
  return {};
}

QList<QString> devicePlannerForm::getSelectedDevice() const {
  QList<QString> listRtn;
  listRtn.append(ui->m_comboBoxSeries->currentText());
  listRtn.append(ui->m_comboBoxFamily->currentText());
  listRtn.append(ui->m_comboBoxPackage->currentText());

  if (m_selectmodel->hasSelection() &&
      !m_selectmodel->selectedRows(0).isEmpty()) {
    auto index = m_selectmodel->selectedRows(0).first();
    listRtn.append(m_model->data(index).toString());
  } else {
    listRtn.append(m_model->data(m_model->index(0, 0)).toString());
  }

  return listRtn;
}

void devicePlannerForm::updateUi(ProjectManager *pm) {
  if (!pm) return;
  pm->setCurrentRun(DEFAULT_FOLDER_SYNTH);
  auto series = pm->getSynthOption(PROJECT_PART_SERIES);
  auto family = pm->getSynthOption(PROJECT_PART_FAMILY);
  auto package = pm->getSynthOption(PROJECT_PART_PACKAGE);

  if (!series.isEmpty() && !family.isEmpty() && !package.isEmpty()) {
    // The order is important here since every combo depends on previous
    // selection
    ui->m_comboBoxSeries->setCurrentIndex(
        ui->m_comboBoxSeries->findData(series, Qt::DisplayRole));
    ui->m_comboBoxFamily->setCurrentIndex(
        ui->m_comboBoxFamily->findData(family, Qt::DisplayRole));
    ui->m_comboBoxPackage->setCurrentIndex(
        ui->m_comboBoxPackage->findData(package, Qt::DisplayRole));
  }

  auto device = pm->getSynthOption(PROJECT_PART_DEVICE);
  auto items = m_model->findItems(device);
  if (!items.isEmpty()) {
    auto index = items.first()->index();
    UpdateSelection(index);
  }
}

void devicePlannerForm::onSeriestextChanged(const QString &arg1) {
  Q_UNUSED(arg1);
  UpdateFamilyComboBox();
}

void devicePlannerForm::onFamilytextChanged(const QString &arg1) {
  Q_UNUSED(arg1);
  UpdatePackageComboBox();
}

void devicePlannerForm::onPackagetextChanged(const QString &arg1) {
  Q_UNUSED(arg1);
  UpdateDeviceTableView();
}

void devicePlannerForm::InitSeriesComboBox() {
  disconnect(ui->m_comboBoxSeries, &QComboBox::currentTextChanged, this,
             &devicePlannerForm::onSeriestextChanged);

  ui->m_comboBoxSeries->clear();

  QList<QString> lisSeries = Config::Instance()->getSerieslist();
  for (int i = 0; i < lisSeries.size(); ++i) {
    ui->m_comboBoxSeries->addItem(lisSeries[i]);
  }

  UpdateFamilyComboBox();
  connect(ui->m_comboBoxSeries, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onSeriestextChanged);
}

void devicePlannerForm::InitDeviceTableViewHead() {
  QList<QString> listHead = Config::Instance()->getDeviceItem();
  for (int i = 0; i < listHead.size(); ++i) {
    m_model->setHorizontalHeaderItem(i, new QStandardItem(listHead.at(i)));
  }
}

void devicePlannerForm::UpdateFamilyComboBox() {
  disconnect(ui->m_comboBoxFamily, &QComboBox::currentTextChanged, this,
             &devicePlannerForm::onFamilytextChanged);

  ui->m_comboBoxFamily->clear();

  QList<QString> lisFamily =
      Config::Instance()->getFamilylist(ui->m_comboBoxSeries->currentText());
  for (int i = 0; i < lisFamily.size(); ++i) {
    ui->m_comboBoxFamily->addItem(lisFamily[i]);
  }

  UpdatePackageComboBox();
  connect(ui->m_comboBoxFamily, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onFamilytextChanged);
}

void devicePlannerForm::UpdatePackageComboBox() {
  disconnect(ui->m_comboBoxPackage, &QComboBox::currentTextChanged, this,
             &devicePlannerForm::onPackagetextChanged);

  ui->m_comboBoxPackage->clear();

  QList<QString> lisPackage = Config::Instance()->getPackagelist(
      ui->m_comboBoxSeries->currentText(), ui->m_comboBoxFamily->currentText());
  for (int i = 0; i < lisPackage.size(); ++i) {
    ui->m_comboBoxPackage->addItem(lisPackage[i]);
  }

  UpdateDeviceTableView();
  connect(ui->m_comboBoxPackage, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onPackagetextChanged);
}

void devicePlannerForm::UpdateDeviceTableView() {
  m_model->clear();
  InitDeviceTableViewHead();
  QList<QStringList> listDevice = Config::Instance()->getDevicelist(
      ui->m_comboBoxSeries->currentText(), ui->m_comboBoxFamily->currentText(),
      ui->m_comboBoxPackage->currentText());
  for (int i = 0; i < listDevice.count(); ++i) {
    int rows = m_model->rowCount();
    QList<QStandardItem *> items;
    QStandardItem *item = nullptr;

    QList<QString> listDev = listDevice.at(i);
    foreach (QString strItem, listDev) {
      item = new QStandardItem(strItem);
      item->setTextAlignment(Qt::AlignCenter);
      items.append(item);
    }

    m_model->insertRow(rows, items);
  }
  UpdateSelection(m_selectmodel->model()->index(0, 0));
}

void devicePlannerForm::UpdateSelection(const QModelIndex &index) {
  m_selectmodel->select(index,
                        QItemSelectionModel::SelectionFlag::ClearAndSelect |
                            QItemSelectionModel::Rows);
}

void devicePlannerForm::init(const Filters &filter) {
  std::string devicefile = (Config::Instance()->dataPath() /
                            std::string("etc") / std::string("device.xml"))
                               .string();
  if (!m_deviceFile.empty()) devicefile = m_deviceFile.string();
  QStringList deviceXmls = {QString::fromStdString(devicefile)};
  auto localDevices = Config::Instance()->customDeviceXml();
  if (FileUtils::FileExists(localDevices))
    deviceXmls.append(QString::fromStdString(localDevices.string()));
  if (0 == Config::Instance()->InitConfigs(deviceXmls)) {
    InitSeriesComboBox();
  }
  m_originalDeviceList = getOriginalDeviceList();
  int index = ui->m_comboBoxSeries->findData(filter.series, Qt::DisplayRole);
  if (index != -1) ui->m_comboBoxSeries->setCurrentIndex(index);
  index = ui->m_comboBoxFamily->findData(filter.family, Qt::DisplayRole);
  if (index != -1) ui->m_comboBoxFamily->setCurrentIndex(index);
  index = ui->m_comboBoxPackage->findData(filter.package, Qt::DisplayRole);
  if (index != -1) ui->m_comboBoxPackage->setCurrentIndex(index);
}

QStringList devicePlannerForm::getOriginalDeviceList() const {
  std::string devicefile = (Config::Instance()->dataPath() /
                            std::string("etc") / std::string("device.xml"))
                               .string();
  if (!m_deviceFile.empty()) devicefile = m_deviceFile.string();
  Config conf{};
  conf.InitConfigs({QString::fromStdString(devicefile)});
  auto deviceInfo = conf.getDevicelist();
  QStringList devices{};
  for (const auto &info : deviceInfo) {
    if (!info.isEmpty()) devices.push_back(info.first());
  }
  return devices;
}

Filters devicePlannerForm::currentFilter() const {
  Filters filter{};
  filter.family = ui->m_comboBoxFamily->currentText();
  filter.package = ui->m_comboBoxPackage->currentText();
  filter.series = ui->m_comboBoxSeries->currentText();
  return filter;
}

void devicePlannerForm::createDevice() {
  auto selectedDevice = selectedDeviceName();
  std::filesystem::path devicePath =
      Config::Instance()->dataPath() / std::string("etc");
  devicePath = devicePath / "devices" / "custom_layout_template.xml";
  CustomLayoutBuilder layoutBuilder{
      {}, QString::fromStdString(devicePath.string())};
  const auto &[ok, string] = layoutBuilder.testTemplateFile();
  if (!ok) {
    QMessageBox::critical(this, "Failed to generate custom layout", string);
    return;
  }
  auto allDevicesList = Config::Instance()->getDevicelist();
  QStringList allDevices{};
  for (const auto &dev : allDevicesList) allDevices.push_back(dev.first());

  auto layout = new CustomLayout{m_originalDeviceList, allDevices, this};
  layout->setBaseDevice(selectedDevice);
  layout->setAttribute(Qt::WA_DeleteOnClose);
  layout->setWindowModality(Qt::ApplicationModal);
  auto filter = currentFilter();
  connect(
      layout, &CustomLayout::sendCustomLayoutData, this,
      [this, devicePath, filter](const FOEDAG::CustomLayoutData &data) {
        CustomLayoutBuilder layoutBuilder{
            data, QString::fromStdString(devicePath.string())};

        const auto &[ok, string] = layoutBuilder.generateCustomLayout();
        if (!ok) {
          QMessageBox::critical(this, "Failed to generate custom layout",
                                string);
          return;
        } else {
          const auto &[success, errorMessage] =
              CustomLayoutBuilder::saveCustomLayout(
                  Config::Instance()->layoutsPath(), data.name + ".xml",
                  string);
          if (!success) {
            QMessageBox::critical(this, "Failed to generate custom layout",
                                  errorMessage);
            return;
          }
        }
        std::string devicefile =
            (Config::Instance()->dataPath() / "etc" / "device.xml").string();
        if (!m_deviceFile.empty()) devicefile = m_deviceFile.string();
        auto localDevices = Config::Instance()->customDeviceXml();

        const auto &[created, message] = layoutBuilder.generateNewDevice(
            QString::fromStdString(devicefile),
            QString::fromStdString(localDevices.string()), data.baseName);
        if (!created) {
          QMessageBox::critical(
              this, QString{"Failed to create new device %1"}.arg(data.name),
              message);
          return;
        }

        // regenerate list
        init(filter);
        // select device user just created
        auto items = m_model->findItems(data.name);
        if (!items.isEmpty()) {
          auto index = items.first()->index();
          UpdateSelection(index);
        }
      });
  layout->open();
}

void devicePlannerForm::updateEditDeviceButtons() {
  bool enable{false};
  auto selectedDevice = selectedDeviceName();
  if (!selectedDevice.isEmpty()) {
    enable = !m_originalDeviceList.contains(selectedDevice);
  }
  ui->pushButtonEdit->setEnabled(enable);
  ui->pushButtonRemove->setEnabled(enable);
}

void devicePlannerForm::removeDevice() {
  auto selectedDevice = selectedDeviceName();
  if (!selectedDevice.isEmpty() &&
      !m_originalDeviceList.contains(selectedDevice)) {
    auto result = QMessageBox::question(
        this, "Remove custom device",
        QString{"Are you sure you want to remove <b>%1</b> device"}.arg(
            selectedDevice),
        QMessageBox::Yes | QMessageBox::No);
    if (result != QMessageBox::Yes) return;

    auto filter = currentFilter();
    const auto &[ok, message] = CustomLayoutBuilder::removeDevice(
        QString::fromStdString(Config::Instance()->customDeviceXml().string()),
        Config::Instance()->layoutsPath(), selectedDevice);
    if (ok) {
      init(filter);
    } else {
      QMessageBox::critical(
          this, QString{"Failed to remove device %1"}.arg(selectedDevice),
          message);
    }
  }
}

void devicePlannerForm::editDevice() {
  auto modifyDevice = selectedDeviceName();
  if (modifyDevice.isEmpty()) return;

  std::filesystem::path devicePath =
      Config::Instance()->dataPath() / std::string("etc");
  devicePath = devicePath / "devices" / "custom_layout_template.xml";
  CustomLayoutBuilder layoutBuilder{
      {}, QString::fromStdString(devicePath.string())};
  const auto &[ok, string] = layoutBuilder.testTemplateFile();
  if (!ok) {
    QMessageBox::critical(this, "Failed to generate custom layout", string);
    return;
  }
  auto allDevicesList = Config::Instance()->getDevicelist();
  QStringList allDevices{};
  for (const auto &dev : allDevicesList) allDevices.push_back(dev.first());
  allDevices.removeIf(
      [modifyDevice](const QString &dev) { return modifyDevice == dev; });

  auto layout = new CustomLayout{m_originalDeviceList, allDevices, this};
  layout->setWindowTitle(QString{"Edit %1 device"}.arg(modifyDevice));
  layout->setAttribute(Qt::WA_DeleteOnClose);
  layout->setWindowModality(Qt::ApplicationModal);
  CustomLayoutData editData;
  auto customLayoutPath =
      Config::Instance()->layoutsPath() / (modifyDevice.toStdString() + ".xml");
  const auto &[loadFromFile, loadFromFileMessage] =
      CustomLayoutBuilder::fromFile(
          QString::fromStdString(customLayoutPath.string()),
          QString::fromStdString(
              Config::Instance()->customDeviceXml().string()),
          editData);
  if (!loadFromFile) {
    QMessageBox::critical(
        this,
        QString{"Failed to modify layout for device %1"}.arg(modifyDevice),
        loadFromFileMessage);
    return;
  }
  layout->setCustomLayoutData(editData);
  auto filter = currentFilter();
  connect(layout, &CustomLayout::sendCustomLayoutData, this,
          [this, devicePath, modifyDevice, customLayoutPath,
           filter](const FOEDAG::CustomLayoutData &data) {
            CustomLayoutBuilder layoutBuilder{
                data, QString::fromStdString(devicePath.string())};

            const auto &[ok, string] = layoutBuilder.generateCustomLayout();
            if (!ok) {
              QMessageBox::critical(this, "Failed to generate custom layout",
                                    string);
              return;
            } else {
              const auto &[saveLayout, errorMessage] =
                  CustomLayoutBuilder::saveCustomLayout(
                      Config::Instance()->layoutsPath(), data.name + ".xml",
                      string);
              if (!saveLayout) {
                QMessageBox::critical(this, "Failed to edit custom layout",
                                      errorMessage);
                return;
              }
            }
            const auto &[modify, modifyErrorMsg] = layoutBuilder.modifyDevice(
                QString::fromStdString(
                    Config::Instance()->customDeviceXml().string()),
                modifyDevice);
            if (!modify) {
              QMessageBox::critical(this, "Failed to modify device",
                                    modifyErrorMsg);
            } else {
              // cleanup file with old name
              if (modifyDevice != data.name) {
                FileUtils::removeFile(customLayoutPath);
              }

              init(filter);
              auto items = m_model->findItems(data.name);
              if (!items.isEmpty()) {
                auto index = items.first()->index();
                UpdateSelection(index);
              }
            }
          });

  layout->open();
}
