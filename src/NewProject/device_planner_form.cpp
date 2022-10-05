#include "device_planner_form.h"

#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QTextStream>
#include <filesystem>
#include <iostream>

#include "ProjectManager/config.h"
#include "ProjectManager/project_manager.h"
#include "ui_device_planner_form.h"

using namespace FOEDAG;

devicePlannerForm::devicePlannerForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::devicePlannerForm) {
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
  const std::string separator(1, std::filesystem::path::preferred_separator);
  std::string devicefile = Config::Instance()->dataPath().string() + separator +
                           std::string("etc") + separator +
                           std::string("device.xml");
  QString devicexml = devicefile.c_str();
  if (0 == Config::Instance()->InitConfig(devicexml)) {
    InitSeriesComboBox();
  }
}

devicePlannerForm::~devicePlannerForm() { delete ui; }

QList<QString> devicePlannerForm::getSelectedDevice() const {
  QList<QString> listRtn;
  listRtn.append(ui->m_comboBoxSeries->currentText());
  listRtn.append(ui->m_comboBoxFamily->currentText());
  listRtn.append(ui->m_comboBoxPackage->currentText());

  if (m_selectmodel->hasSelection()) {
    int curRow = m_selectmodel->currentIndex().row();
    listRtn.append(m_model->data(m_model->index(curRow, 0)).toString());
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

  if (series.isEmpty() || family.isEmpty() || package.isEmpty()) return;
  // The order is important here since every combo depends on previous selection
  ui->m_comboBoxSeries->setCurrentIndex(
      ui->m_comboBoxSeries->findData(series, Qt::DisplayRole));
  ui->m_comboBoxFamily->setCurrentIndex(
      ui->m_comboBoxFamily->findData(family, Qt::DisplayRole));
  ui->m_comboBoxPackage->setCurrentIndex(
      ui->m_comboBoxPackage->findData(package, Qt::DisplayRole));
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
}
