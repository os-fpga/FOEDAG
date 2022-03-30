#include "runs_options_form.h"

#include "ui_runs_options_form.h"

using namespace FOEDAG;

RunsOptionsForm::RunsOptionsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::RunsOptionsForm) {
  ui->setupUi(this);

  ui->m_labelTitle->setText(tr("Launch Options"));
  ui->m_labelDetail->setText(tr("Set launch options. "));
  ui->m_labelOption->setText(tr("Options:"));
  ui->m_radioBtnLaunch->setText(tr("  Launch runs on local host."));
  ui->m_radioBtnDonot->setText(tr("  Do not launch now."));

  ui->m_radioBtnLaunch->setChecked(true);
}

RunsOptionsForm::~RunsOptionsForm() { delete ui; }
