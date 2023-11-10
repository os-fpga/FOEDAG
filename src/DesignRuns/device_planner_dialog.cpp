#include "device_planner_dialog.h"

#include <QVBoxLayout>

#include "ui_device_planner_dialog.h"

using namespace FOEDAG;

DevicePlannerDialog::DevicePlannerDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::DevicePlannerDialog) {
  ui->setupUi(this);

  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  setWindowTitle(tr("Device Planner"));

  m_deviceForm = new devicePlannerForm(this);

  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_widgetForm);
  box->addWidget(m_deviceForm);
  box->setContentsMargins(0, 0, 0, 1);
  box->setSpacing(0);
  ui->m_widgetForm->setLayout(box);

  this->setResult(0);
}

DevicePlannerDialog::~DevicePlannerDialog() { delete ui; }

QString DevicePlannerDialog::getSelectedDevice() {
  QStringList strlist = m_deviceForm->getSelectedDevice();
  QString strDevice = "";
  if (strlist.count() > 3) {
    strDevice = strlist.at(3);
  }
  return strDevice;
}

QString DevicePlannerDialog::getSelectedPackage() {
  QStringList strlist = m_deviceForm->getSelectedDevice();
  QString strPackage = "";
  if (strlist.count() > 2) {
    strPackage = strlist.at(2);
  }
  return strPackage;
}

void DevicePlannerDialog::on_m_btnOK_clicked() {
  this->setResult(1);
  this->hide();
}

void DevicePlannerDialog::on_m_btnCancel_clicked() { this->hide(); }
