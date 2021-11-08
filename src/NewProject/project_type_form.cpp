#include "project_type_form.h"

#include "ui_project_type_form.h"

projectTypeForm::projectTypeForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::projectTypeForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Project Type"));
  ui->m_labelDetail->setText(tr("Specify the type of project to create."));
  ui->m_radioBtnRTL->setText(tr("RTL Project"));
  ui->m_radioBtnPost->setText(tr("Post-synthesis Project"));
  ui->m_labelRTL->setText(
      tr("You will be able to add sources,create block designs in IP "
         "integrator,generate IP, "
         "run RTL analysis,synthesis,implementation,design planning and "
         "analysis."));
  ui->m_labelPost->setText(
      tr("You will be able to add sources, view device resources,run design "
         "analysis,planning and implementation."));

  ui->m_radioBtnRTL->setChecked(true);
}

projectTypeForm::~projectTypeForm() { delete ui; }
int projectTypeForm::getprojecttype() {
  return ui->m_radioBtnRTL->isChecked() ? TYPE_RTL : TYPE_POST;
}
bool projectTypeForm::isaddsource() { return true; }

void projectTypeForm::on_m_radioBtnRTL_clicked() { updatestate(); }

void projectTypeForm::on_m_radioBtnPost_clicked() { updatestate(); }
void projectTypeForm::updatestate() {}
