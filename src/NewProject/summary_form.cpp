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

void summaryForm::setProjectSettings(bool on) {
  m_projectSettings = on;
  ui->m_labelTitle->setText(on ? tr("Project Summary")
                               : tr("New Project Summary"));
  ui->m_labelTail->setVisible(!on);
}

void summaryForm::setProjectName(const QString &proName,
                                 const QString &proType) {
  ui->m_labelNamePic->setStyleSheet(QString("image: url(:/img/info.png);"));
  QString text = m_projectSettings
                     ? tr("%1 project named '%2'")
                     : tr("A new %1 project named '%2' will be created:");
  ui->m_labelName->setText(text.arg(proType, proName));
}

void summaryForm::setSourceCount(const int &srcCount, const int constrCount,
                                 int simCount) {
  if (0 == srcCount) {
    ui->m_labelSourcesPic->setStyleSheet(
        QString("image: url(:/img/warn.png);"));
    ui->m_labelSources->setText(m_projectSettings
                                    ? tr("No source files or directories.")
                                    : tr("No source files or directories will "
                                         "be added. Use Add Sources to "
                                         "add them later."));
  } else {
    ui->m_labelSourcesPic->setStyleSheet(
        QString("image: url(:/img/info.png);"));
    ui->m_labelSources->setText(
        m_projectSettings
            ? QString(tr("%1 source files.")).arg(srcCount)
            : QString(tr("%1 source files will be added.")).arg(srcCount));
  }

  if (0 == simCount) {
    ui->m_labelSimPic->setVisible(false);
    ui->m_labelSim->setVisible(false);
    // ui->m_labelSimPic->setStyleSheet(QString("image: url(:/img/warn.png);"));
    // ui->m_labelSim->setText(m_projectSettings
    //                             ? tr("No simulation files or directories.")
    //                             : tr("No simulation files or directories will "
    //                                  "be added. Use Add Sources to "
    //                                  "add them later."));
  } else {
    ui->m_labelSimPic->setStyleSheet(QString("image: url(:/img/info.png);"));
    ui->m_labelSim->setText(
        m_projectSettings
            ? QString(tr("%1 simulation files.")).arg(simCount)
            : QString(tr("%1 simulation files will be added.")).arg(simCount));
  }

  if (0 == constrCount) {
    ui->m_labelConstraintsPic->setVisible(false);
    ui->m_labelConstraints->setVisible(false);
    // ui->m_labelConstraintsPic->setStyleSheet(
    //     QString("image: url(:/img/warn.png);"));
    // ui->m_labelConstraints->setText(
    //     m_projectSettings ? tr("No constraints files.")
    //                       : tr("No constraints files will be added. Use Add "
    //                            "Sources to add them "
    //                            "later."));
  } else {
    ui->m_labelConstraintsPic->setStyleSheet(
        QString("image: url(:/img/info.png);"));
    ui->m_labelConstraints->setText(
        m_projectSettings
            ? QString(tr("%1 constraints files.")).arg(constrCount)
            : QString(tr("%1 constraints files will be added."))
                  .arg(constrCount));
  }
}

void summaryForm::setDeviceInfo(const QStringList &listDevItem) {
//   if (listDevItem.count() > 3) {
//     auto itemIndex = 0;
//     for (const auto &txt : listDevItem) {
//       auto item = new QTableWidgetItem(txt);
//       item->setTextAlignment(Qt::AlignCenter);
//       ui->m_deviceInfoTable->setItem(0, itemIndex, item);
//       ++itemIndex;
//     }
//   }
}
