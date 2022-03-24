#include "new_project_dialog.h"

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

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 3;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);

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
  m_projectManager->CreateProject(m_locationForm->getProjectName(),
                                  m_locationForm->getProjectPath());

  m_projectManager->setProjectType(m_proTypeForm->getProjectType());

  m_projectManager->setCurrentFileSet(DEFAULT_FOLDER_SOURCE);
  QString strDefaultSrc = "";
  QList<filedata> listFile = m_addSrcForm->getFileData();
  foreach (filedata fdata, listFile) {
    if ("<Local to Project>" == fdata.m_filePath) {
      m_projectManager->setDesignFile(fdata.m_fileName, false);
    } else {
      m_projectManager->setDesignFile(fdata.m_filePath + "/" + fdata.m_fileName,
                                      m_addSrcForm->IsCopySource());
    }
    if (!fdata.m_isFolder) {
      strDefaultSrc = fdata.m_fileName;
    }
  }

  if ("" != strDefaultSrc) {
    QString module = strDefaultSrc.left(strDefaultSrc.lastIndexOf("."));
    m_projectManager->setTopModule(module);

    // set default simulation source
    m_projectManager->setCurrentFileSet(DEFAULT_FOLDER_SIM);
    m_projectManager->setDesignFile("sim_" + strDefaultSrc, false);
    m_projectManager->setTopModule("sim_" + module);
  }

  m_projectManager->setCurrentFileSet(DEFAULT_FOLDER_CONSTRS);
  QString strDefaultCts = "";
  listFile.clear();
  listFile = m_addConstrsForm->getFileData();
  foreach (filedata fdata, listFile) {
    if ("<Local to Project>" == fdata.m_filePath) {
      m_projectManager->setConstrsFile(fdata.m_fileName, false);
    } else {
      m_projectManager->setConstrsFile(
          fdata.m_filePath + "/" + fdata.m_fileName,
          m_addConstrsForm->IsCopySource());
    }
    strDefaultCts = fdata.m_fileName;
  }

  if ("" != strDefaultCts) {
    m_projectManager->setTargetConstrs(strDefaultCts);
  }

  m_projectManager->setCurrentRun(DEFAULT_FOLDER_SYNTH);

  QStringList strlist = m_devicePlanForm->getSelectedDevice();
  QList<QPair<QString, QString>> listParam;
  QPair<QString, QString> pair;
  pair.first = PROJECT_PART_SERIES;
  pair.second = strlist.at(0);
  listParam.append(pair);
  pair.first = PROJECT_PART_FAMILY;
  pair.second = strlist.at(1);
  listParam.append(pair);
  pair.first = PROJECT_PART_PACKAGE;
  pair.second = strlist.at(2);
  listParam.append(pair);
  pair.first = PROJECT_PART_DEVICE;
  pair.second = strlist.at(3);
  listParam.append(pair);
  m_projectManager->setSynthesisOption(listParam);

  m_projectManager->FinishedProject();
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
