#include "summary_form.h"

#include "project_type_form.h"
#include "ui_summary_form.h"

using namespace FOEDAG;

summaryForm::summaryForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::summaryForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("New Project Summary"));
  ui->m_labelTail->setText(tr("To create the project, click Finish."));
}

summaryForm::~summaryForm() { delete ui; }
void summaryForm::setProjectName(const QString &proName,
                                 const QString &proType) {
  ui->m_labelNamePic->setStyleSheet(QString("image: url(:/img/info.png);"));
  ui->m_labelName->setText(
      QString(tr("A new %1 project named '%2' will be created:"))
          .arg(proType)
          .arg(proName));
}

void summaryForm::setSourceCount(const int &srcCount, const int constrCount) {
  if (0 == srcCount) {
    ui->m_labelSourcesPic->setStyleSheet(
        QString("image: url(:/img/warn.png);"));
    ui->m_labelSources->setText(
        tr("No source files or directories will be added. Use Add Sources to "
           "add them later."));
  } else {
    ui->m_labelSourcesPic->setStyleSheet(
        QString("image: url(:/img/info.png);"));
    ui->m_labelSources->setText(
        QString(tr("%1 source files will be added.")).arg(srcCount));
  }

  if (0 == constrCount) {
    ui->m_labelConstraintsPic->setStyleSheet(
        QString("image: url(:/img/warn.png);"));
    ui->m_labelConstraints->setText(
        tr("No constraints files will be added. Use Add Sources to add them "
           "later."));
  } else {
    ui->m_labelConstraintsPic->setStyleSheet(
        QString("image: url(:/img/info.png);"));
    ui->m_labelConstraints->setText(
        QString(tr("%1 constraints files will be added.")).arg(constrCount));
  }
}

void summaryForm::setDeviceInfo(const QStringList &listDevItem) {
  if (listDevItem.count() > 3) {
    auto itemIndex = 0;
    for (const auto &txt : listDevItem) {
      auto item = new QTableWidgetItem(txt);
      item->setTextAlignment(Qt::AlignCenter);
      ui->m_deviceInfoTable->setItem(0, itemIndex, item);
      ++itemIndex;
    }
  }
}
