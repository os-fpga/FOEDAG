#include "add_constraints_form.h"

#include <QFileInfo>

#include "Compiler/Compiler.h"
#include "MainWindow/Session.h"
#include "ProjectManager/project_manager.h"
#include "ui_add_constraints_form.h"

extern FOEDAG::Session *GlobalSession;

using namespace FOEDAG;

addConstraintsForm::addConstraintsForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addConstraintsForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Design Constraints (optional)"));
  ui->m_labelDetail->setText(
      tr("Specify or create constraint file for physical and timing "
         "constraints."));

  m_widgetGrid = new sourceGrid(ui->m_frame);
  m_widgetGrid->setGridType(GT_CONSTRAINTS);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project."));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Unchecked);

  Compiler *compiler = GlobalSession->GetCompiler();
  if (compiler->PinAssignOpts() == Compiler::PinAssignOpt::Random)
    ui->select_random->setChecked(true);
  else if (compiler->PinAssignOpts() ==
           Compiler::PinAssignOpt::Pin_constraint_disabled)
    ui->select_free->setChecked(true);
  else
    ui->select_defineOrder->setChecked(true);
}

addConstraintsForm::~addConstraintsForm() { delete ui; }

QList<filedata> addConstraintsForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool addConstraintsForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}

bool addConstraintsForm::IsRandom() const {
  return ui->select_random->isChecked();
}

bool addConstraintsForm::IsFree() const { return ui->select_free->isChecked(); }

void addConstraintsForm::updateUi(ProjectManager *pm) {
  if (!pm) return;
  pm->setCurrentFileSet(DEFAULT_FOLDER_CONSTRS);
  auto activeSet = pm->getConstrActiveFileSet();
  const auto files = pm->getConstrFiles(activeSet);
  m_widgetGrid->ClearTable();
  for (const auto &file : files) {
    const QFileInfo fileInfo{file};
    filedata data;
    data.m_isFolder = false;
    data.m_fileName = fileInfo.fileName();
    data.m_fileType = fileInfo.suffix();
    // TODO @volodymyrK temp solution, this should displayed as <Local to
    // Project>
    if (fileInfo.path().startsWith(PROJECT_OSRCDIR))
      data.m_filePath =
          fileInfo.path().replace(PROJECT_OSRCDIR, pm->getProjectPath());
    else
      data.m_filePath = fileInfo.path();

    m_widgetGrid->AddTableItem(data);
  }
}

void addConstraintsForm::SetTitle(const QString &title) {
  ui->m_labelTitle->setText(title);
}
