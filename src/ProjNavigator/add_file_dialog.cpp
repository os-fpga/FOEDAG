#include "add_file_dialog.h"

#include <QDesktopWidget>

#include "Compiler/Compiler.h"
#include "Compiler/CompilerDefines.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ui_add_file_dialog.h"

extern FOEDAG::Session *GlobalSession;
using namespace FOEDAG;

AddFileDialog::AddFileDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::AddFileDialog),
      m_formIndex(INDEX_TYPESELECT) {
  ui->setupUi(this);
  setWindowTitle(tr("Add Sources"));

  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

  m_selectForm = new SelectFileTypeForm(this);
  ui->m_stackedWidget->insertWidget(1, m_selectForm);
  m_fileForm = new AddFileForm(this);
  ui->m_stackedWidget->insertWidget(2, m_fileForm);
  ui->m_stackedWidget->adjustSize();

  UpdateDialogView();

  m_pm = new ProjectManager(this);
}

AddFileDialog::~AddFileDialog() { delete ui; }

void AddFileDialog::setSelected(int iSelected) {
  m_selectForm->setSelectedType(iSelected);
}

void AddFileDialog::on_m_btnOK_clicked() {
  int ret = 0;
  int iType = m_selectForm->getSelectedType();
  if (m_formIndex == INDEX_FILEFORM) {
    m_pm->setCurrentFileSet(m_fileForm->getFileSet());
    const QList<filedata> listFile = m_fileForm->getFileData();
    for (const filedata &fdata : listFile) {
      if ("<Local to Project>" == fdata.m_filePath) {
        if (GT_SOURCE == iType) {
          // TODO RG-132 @volodymyrk. If group exists we should append to list
          ret = m_pm->setDesignFiles({}, {}, fdata.m_fileName, fdata.m_language,
                                     fdata.m_groupName, false, true);
        } else if (GT_CONSTRAINTS == iType) {
          ret = m_pm->setConstrsFile(fdata.m_fileName, false, true);
        } else if (GT_SIM == iType) {
          ret = m_pm->setSimulationFile(fdata.m_fileName, false, true);
        }
      } else {
        if (GT_SOURCE == iType) {
          // TODO RG-132 @volodymyrk. If group exists we should append to list
          ret = m_pm->setDesignFiles({}, {},
                                     fdata.m_filePath + "/" + fdata.m_fileName,
                                     fdata.m_language, fdata.m_groupName,
                                     m_fileForm->IsCopySource(), false);
        } else if (GT_CONSTRAINTS == iType) {
          ret = m_pm->setConstrsFile(fdata.m_filePath + "/" + fdata.m_fileName,
                                     m_fileForm->IsCopySource(), false);
        } else if (GT_SIM == iType) {
          ret =
              m_pm->setSimulationFile(fdata.m_filePath + "/" + fdata.m_fileName,
                                      m_fileForm->IsCopySource(), false);
        }
      }
    }
    Compiler *compiler = GlobalSession->GetCompiler();
    if (m_fileForm->IsRandom())
      compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
    else if (m_fileForm->IsFree())
      compiler->PinAssignOpts(Compiler::PinAssignOpt::Pin_constraint_disabled);
    else
      compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
    this->close();
    if (0 == ret) {
      m_pm->FinishedProject();
      emit RefreshFiles();
    }
  } else {
    m_formIndex++;
    m_fileForm->InitForm(iType);
    UpdateDialogView();
  }
}

void AddFileDialog::on_m_btnCancel_clicked() { this->close(); }
void AddFileDialog::on_m_btnBack_clicked() {
  m_formIndex--;
  UpdateDialogView();
}

void AddFileDialog::UpdateDialogView() {
  if (m_formIndex == INDEX_TYPESELECT) {
    ui->m_btnBack->setEnabled(false);
    ui->m_btnOK->setText(tr("Next"));
  } else if (m_formIndex == INDEX_FILEFORM) {
    ui->m_btnBack->setEnabled(true);
    ui->m_btnOK->setText(tr("Finish"));
  }
  ui->m_stackedWidget->setCurrentIndex(m_formIndex);
}
