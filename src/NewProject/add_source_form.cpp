#include "add_source_form.h"

#include <QFileInfo>

#include "ProjectManager/project_manager.h"
#include "ui_add_source_form.h"

using namespace FOEDAG;

addSourceForm::addSourceForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addSourceForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Design Files"));
  ui->m_labelDetail->setText(tr(
      "Specify design files, or directories containing those files, to add to "
      "your project. "
      "Create a new source file on disk and add it to your project. "));

  m_widgetGrid = new sourceGrid(ui->m_frame);
  m_widgetGrid->setGridType(GT_SOURCE);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project. "));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Unchecked);
}

addSourceForm::~addSourceForm() { delete ui; }

QList<filedata> addSourceForm::getFileData() {
  return m_widgetGrid->getTableViewData();
}

bool addSourceForm::IsCopySource() {
  return ui->m_ckkBoxCopy->checkState() == Qt::CheckState::Checked ? true
                                                                   : false;
}

QString addSourceForm::TopModule() const {
  return ui->lineEditTopModule->text().trimmed();
}

QString addSourceForm::LibraryForTopModule() const {
  return ui->lineEditTopModuleLib->text().trimmed();
}

QString addSourceForm::IncludePath() const {
  return ui->lineEditIncludePath->text().trimmed();
}

QString addSourceForm::LibraryPath() const {
  return ui->lineEditLibraryPath->text().trimmed();
}

QString addSourceForm::LibraryExt() const {
  return ui->lineEditLibraryExt->text().trimmed();
}

QString addSourceForm::Macros() const {
  return ui->lineEditSetMacro->text().trimmed();
}

void addSourceForm::updateUi(ProjectManager *pm) {
  if (!pm) return;

  ui->lineEditTopModule->setText(pm->getDesignTopModule());
  ui->lineEditTopModuleLib->setText(pm->getDesignTopModuleLib());

  m_widgetGrid->ClearTable();
  auto libs = pm->DesignLibraries();
  int index{0};
  for (const auto &lang_file : pm->DesignFiles()) {
    filedata data;
    data.m_language = lang_file.first;
    if (index < libs.size()) {
      if (!libs.at(index).first.empty()) {
        if (!libs.at(index).second.empty())
          data.m_workLibrary =
              QString::fromStdString(libs.at(index).second.front());
      }
    }
    const QStringList fileList =
        QString::fromStdString(lang_file.second).split(" ");
    for (const auto &file : fileList) {
      const QFileInfo info{file};
      data.m_fileName = info.fileName();
      data.m_filePath = info.path();
      data.m_isFolder = false;
      data.m_fileType = info.suffix();
      m_widgetGrid->AddTableItem(data);
    }
    index++;
  }
}

void addSourceForm::SetTitle(const QString &title) {
  ui->m_labelTitle->setText(title);
}
