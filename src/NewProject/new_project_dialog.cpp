#include "new_project_dialog.h"

#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QThread>

#include "ui_new_project_dialog.h"
using namespace FOEDAG;

newProjectDialog::newProjectDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::newProjectDialog), m_index(INDEX_LOCATION) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  setWindowTitle(tr("New Project"));

  Reset();

  m_projectManager = new ProjectManager(this);
}

newProjectDialog::~newProjectDialog() { delete ui; }

void newProjectDialog::Next_TclCommand_Test() {
  QThread::sleep(0);
  emit ui->m_btnNext->clicked();
}

void newProjectDialog::CreateProject_Tcl_Test(int argc, const char *argv[]) {
  m_projectManager->Tcl_CreateProject(argc, argv);
}

QString newProjectDialog::getProject() {
  return m_locationForm->getProjectPath() + "/" +
         m_locationForm->getProjectName() + PROJECT_FILE_FORMAT;
}

void newProjectDialog::Reset() {
  m_index = INDEX_LOCATION;

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 3;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);

  for (int i = 0; i < ui->m_stackedWidget->count(); i++) {
    auto w = ui->m_stackedWidget->widget(i);
    ui->m_stackedWidget->removeWidget(w);
    delete w;
  }

  m_locationForm = new locationForm(this);
  ui->m_stackedWidget->insertWidget(1, m_locationForm);
  m_proTypeForm = new projectTypeForm(this);
  ui->m_stackedWidget->insertWidget(2, m_proTypeForm);
  m_addSrcForm = new addSourceForm(this);
  ui->m_stackedWidget->insertWidget(3, m_addSrcForm);
  m_addConstrsForm = new addConstraintsForm(this);
  ui->m_stackedWidget->insertWidget(4, m_addConstrsForm);
  m_devicePlanForm = new devicePlannerForm(this);
  ui->m_stackedWidget->insertWidget(5, m_devicePlanForm);
  m_sumForm = new summaryForm(this);
  ui->m_stackedWidget->insertWidget(6, m_sumForm);
  ui->m_stackedWidget->adjustSize();

  UpdateDialogView();
}

void newProjectDialog::on_m_btnBack_clicked() {
  m_index--;
  UpdateDialogView();
}

void newProjectDialog::on_m_btnNext_clicked() {
  if (INDEX_LOCATION == m_index) {
    if ("" == m_locationForm->getProjectName()) {
      QMessageBox::information(this, tr("Information"),
                               tr("Please specify a project name"),
                               QMessageBox::Ok);
      return;
    }

    if ("" == m_locationForm->getProjectPath()) {
      QMessageBox::information(this, tr("Information"),
                               tr("Please select a path for your project"),
                               QMessageBox::Ok);
      return;
    }
    if (m_locationForm->IsProjectNameExit()) {
      QMessageBox::information(
          this, tr("Information"),
          tr("Project name already exists,Please rename for your project"),
          QMessageBox::Ok);
      return;
    }
  }
  m_index++;
  UpdateDialogView();
}

void newProjectDialog::on_m_btnFinish_clicked() {
  ProjectOptions opt{
      m_locationForm->getProjectName(),
      m_locationForm->getProjectPath(),
      m_proTypeForm->getProjectType(),
      {m_addSrcForm->getFileData(), m_addSrcForm->IsCopySource()},
      {m_addConstrsForm->getFileData(), m_addConstrsForm->IsCopySource()},
      m_devicePlanForm->getSelectedDevice(),
      false /*rewrite*/,
      DEFAULT_FOLDER_SOURCE};
  m_projectManager->CreateProject(opt);
  this->setResult(1);
  this->hide();
}

void newProjectDialog::on_m_btnCancel_clicked() {
  this->setResult(0);
  this->hide();
}
void newProjectDialog::UpdateDialogView() {
  if (INDEX_LOCATION == m_index) {
    ui->m_btnBack->setEnabled(false);
  } else {
    ui->m_btnBack->setEnabled(true);
  }

  if (INDEX_SUMMARYF == m_index) {
    ui->m_btnNext->setEnabled(false);
    ui->m_btnFinish->setEnabled(true);
    m_sumForm->setProjectName(m_locationForm->getProjectName(),
                              m_proTypeForm->getProjectType());
    m_sumForm->setDeviceInfo(m_devicePlanForm->getSelectedDevice());
    m_sumForm->setSourceCount(m_addSrcForm->getFileData().count(),
                              m_addConstrsForm->getFileData().count());
  } else {
    ui->m_btnNext->setEnabled(true);
    ui->m_btnFinish->setEnabled(false);
  }

  ui->m_stackedWidget->setCurrentIndex(m_index);
}
