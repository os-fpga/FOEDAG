#include "runs_summary_form.h"

#include "ui_runs_summary_form.h"

using namespace FOEDAG;

RunsSummaryForm::RunsSummaryForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::RunsSummaryForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Create New Runs Summary"));
  ui->m_labelTail->setText(tr("To create the runs,click Finish."));
}

RunsSummaryForm::~RunsSummaryForm() { delete ui; }

void RunsSummaryForm::setRunsCount(const int &synth, const int &imple) {
  if (-1 == synth) {
    ui->m_labelSynthPic->setVisible(false);
    ui->m_labelSynth->setVisible(false);
  } else {
    ui->m_labelSynthPic->setVisible(true);
    ui->m_labelSynth->setVisible(true);

    ui->m_labelSynthPic->setStyleSheet(
        synth ? QString("image: url(:/img/info.png);")
              : QString("image: url(:/img/warn.png);"));
    ui->m_labelSynth->setText(
        synth ? QString(tr("%1 synthesis runs will be created.")).arg(synth)
              : tr("No synthesis run will be created."));
  }

  if (-1 == imple) {
    ui->m_labelImplePic->setVisible(false);
    ui->m_labelImple->setVisible(false);
  } else {
    ui->m_labelImplePic->setVisible(true);
    ui->m_labelImple->setVisible(true);
    ui->m_labelImplePic->setStyleSheet(
        imple ? QString("image: url(:/img/info.png);")
              : QString("image: url(:/img/warn.png);"));
    ui->m_labelImple->setText(
        imple ? QString(tr("%1 implementation run will be created.")).arg(imple)
              : tr("No implementation run will be created."));
  }
}
