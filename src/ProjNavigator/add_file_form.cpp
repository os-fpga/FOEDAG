#include "add_file_form.h"

#include <QMessageBox>

#include "NewProject/ProjectManager/project_manager.h"
#include "create_fileset_dialog.h"
#include "ui_add_file_form.h"

using namespace FOEDAG;

AddFileForm::AddFileForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::AddFileForm) {
  ui->setupUi(this);

  m_widgetGrid = new sourceGrid(ui->m_frame);

  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  m_pm = new ProjectManager(this);
}

AddFileForm::~AddFileForm() { delete ui; }

void AddFileForm::InitForm(int itype) {
  if (GT_SOURCE == itype) {
    ui->m_labelTitle->setText(tr("Add or Create Design Sources"));
    ui->m_labelDetail->setText(tr(
        "Specify design files,or directories containing those files,to add to "
        "your project."
        "Create a new source file on disk and add it to your project."));
    ui->m_labelSets->setText(tr("Specify design file set:"));
    m_widgetGrid->setGridType(GT_SOURCE);
    ui->m_labelSets->hide();
    ui->m_comboBoxSets->hide();

    ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  } else if (GT_CONSTRAINTS == itype) {
    ui->m_labelTitle->setText(tr("Add or Create Constraints"));
    ui->m_labelDetail->setText(
        tr("Specify or create constraint files for physical and timing "
           "constraints to add to your project."));
    ui->m_labelSets->setText(tr("Specify constraint set:"));

    m_widgetGrid->setGridType(GT_CONSTRAINTS);
    ui->m_ckkBoxCopy->setText(tr("Copy constraints files into project."));
  } else if (GT_SIM == itype) {
    ui->m_labelTitle->setText(tr("Add or Create Simulation Sources"));
    ui->m_labelDetail->setText(
        tr("Specify simulation specific HDL files,or directories containing "
           "HDL files,to add to "
           "your project."
           "Create a new source file on disk and add it to your project."));
    ui->m_labelSets->setText(tr("Specify Simulation set:"));

    m_widgetGrid->setGridType(GT_SOURCE);
    ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  }

  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);

  initSetComboBox(itype);
}

QString AddFileForm::getFileSet() const {
  return ui->m_comboBoxSets->currentText();
}

QList<filedata> AddFileForm::getFileData() const {
  return m_widgetGrid->getTableViewData();
}

bool AddFileForm::IsCopySource() const {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}

void AddFileForm::initSetComboBox(int itype) {
  ui->m_comboBoxSets->clear();

  QStringList listFileSet;
  QString strCreate;
  if (GT_SOURCE == itype) {
    listFileSet = m_pm->getDesignFileSets();
    strCreate = QString(tr("Create Design Set"));
  } else if (GT_CONSTRAINTS == itype) {
    listFileSet = m_pm->getConstrFileSets();
    strCreate = QString(tr("Create Constraint Set"));
  } else if (GT_SIM == itype) {
    listFileSet = m_pm->getSimulationFileSets();
    strCreate = QString(tr("Create Simulation Set"));
  }

  foreach (QString str, listFileSet) {
    ui->m_comboBoxSets->addItem(QIcon(":/images/open-file.png"), str);
  }
  ui->m_comboBoxSets->addItem(strCreate);
}

void AddFileForm::on_m_comboBoxSets_currentIndexChanged(const QString &arg1) {
  int iCreateType;
  if (arg1 == "Create Design Set") {
    iCreateType = FST_DESIGN;
  } else if (arg1 == "Create Constraint Set") {
    iCreateType = FST_CONSTR;
  } else if (arg1 == "Create Simulation Set") {
    iCreateType = FST_SIM;
  } else {
    return;
  }

  int index = ui->m_comboBoxSets->findText(arg1);
  ui->m_comboBoxSets->setCurrentIndex(index - 1);

  CreateFileSetDialog *createdialog = new CreateFileSetDialog(this);
  createdialog->InitDialog(iCreateType);

  while (createdialog->exec()) {
    QString strName = createdialog->getDesignName();
    int ret = 0;
    if (arg1 == "Create Design Set") {
      ret = m_pm->setDesignFileSet(strName);
    } else if (arg1 == "Create Constraint Set") {
      ret = m_pm->setConstrFileSet(strName);
    } else if (arg1 == "Create Simulation Set") {
      ret = m_pm->setSimulationFileSet(strName);
    }
    if (1 == ret) {
      QMessageBox::information(this, tr("Information"),
                               tr("The set name is already exists!"),
                               QMessageBox::Ok);
    } else if (0 != ret) {
      QMessageBox::information(this, tr("Error"), tr("Create set failed!"),
                               QMessageBox::Ok);
      break;
    } else {
      int index = ui->m_comboBoxSets->findText(arg1);
      ui->m_comboBoxSets->removeItem(index);
      ui->m_comboBoxSets->addItem(QIcon(":/images/open-file.png"), strName);
      ui->m_comboBoxSets->addItem(arg1);
      ui->m_comboBoxSets->setCurrentText(strName);
      m_pm->FinishedProject();
      break;
    }
  }
  createdialog->close();
  createdialog->deleteLater();
}
