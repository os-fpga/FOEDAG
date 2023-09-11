#include "add_source_form.h"

#include <QFileDialog>
#include <QFileInfo>

#include "ProjectManager/project_manager.h"
#include "ui_add_source_form.h"

using namespace FOEDAG;

addSourceForm::addSourceForm(GridType gt, QWidget *parent)
    : QWidget(parent), ui(new Ui::addSourceForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Add Design Files"));
  if (gt == GT_SOURCE) {
    ui->m_labelDetail->setText(
        tr("Specify design files, or directories containing those files, to "
           "add to your project. Create a new source file on disk and add it "
           "to your project. "));
  } else if (gt == GT_SIM) {
    ui->m_labelDetail->setText(
        tr("Specify simulation specific files, or directories containing "
           "HDL files, to add to your project. Create a new source file on "
           "disk and add it to your project. "));
  }

  m_widgetGrid = new sourceGrid(ui->m_frame);
  m_widgetGrid->setGridType(gt);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project. "));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Checked);

  connect(ui->toolButtonIncludePath, &QToolButton::clicked, this,
          &addSourceForm::includePathClicked);
  connect(ui->toolButtonLibraryPath, &QToolButton::clicked, this,
          &addSourceForm::libraryPathClicked);
}

addSourceForm::~addSourceForm() { delete ui; }

void addSourceForm::setProjectType(int projectType) {
  m_widgetGrid->setProjectType(projectType);
}

int addSourceForm::projectType() const { return m_widgetGrid->projectType(); }

void addSourceForm::clear() { m_widgetGrid->ClearTable(); }

void addSourceForm::SetBasePath(const QString &p) { m_basePath = p; }

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

  auto fillTable = [this, pm](
                       const ProjectManager::Libraries &libs,
                       const ProjectManager::CompilationUnits &allFiles) {
    int index{0};
    for (const auto &lang_file : allFiles) {
      filedata data;
      data.m_language = lang_file.first.language;
      data.m_groupName = lang_file.first.group;
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
  };

  if (m_widgetGrid->gridType() == GT_SOURCE) {
    ui->lineEditTopModule->setText(pm->getDesignTopModule());
    ui->lineEditTopModuleLib->setText(pm->getDesignTopModuleLib());
    ui->lineEditLibraryExt->setText(pm->libraryExtension());
    ui->lineEditLibraryPath->setText(pm->libraryPath());
    ui->lineEditIncludePath->setText(pm->includePath());
    ui->lineEditSetMacro->setText(pm->macros());

    m_widgetGrid->ClearTable();
    fillTable(pm->DesignLibraries(), pm->DesignFiles());
  } else if (m_widgetGrid->gridType() == GT_SIM) {
    ui->lineEditTopModule->setText(pm->getSimulationTopModule());
    ui->lineEditTopModuleLib->setText(pm->getSimulationTopModuleLib());
    ui->lineEditLibraryExt->setText(pm->simLibraryExtension());
    ui->lineEditLibraryPath->setText(pm->libraryPathSim());
    ui->lineEditIncludePath->setText(pm->includePathSim());
    ui->lineEditSetMacro->setText(pm->macrosSim());

    m_widgetGrid->ClearTable();
    fillTable(pm->SimulationLibraries(), pm->SimulationFiles());
  }
}

void addSourceForm::SetTitle(const QString &title) {
  ui->m_labelTitle->setText(title);
}

void addSourceForm::includePathClicked() {
  auto relatedPath = GetRelatedPath(this, m_basePath);
  if (!relatedPath.isEmpty()) {
    auto actual = ui->lineEditIncludePath->text();
    if (!actual.isEmpty()) actual += " ";
    actual += relatedPath;
    ui->lineEditIncludePath->setText(actual);
  }
}

void addSourceForm::libraryPathClicked() {
  auto relatedPath = GetRelatedPath(this, m_basePath);
  if (!relatedPath.isEmpty()) {
    auto actual = ui->lineEditLibraryPath->text();
    if (!actual.isEmpty()) actual += " ";
    actual += relatedPath;
    ui->lineEditLibraryPath->setText(actual);
  }
}

QString addSourceForm::GetRelatedPath(QWidget *parent, const QString &base) {
  auto folder = QFileDialog::getExistingDirectory(
      parent, tr("Select Directory"), base,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks |
          QFileDialog::DontUseNativeDialog);
  if (folder.isEmpty()) return QString{};

  std::filesystem::path f{folder.toStdString()};
  std::filesystem::path b{base.toStdString()};
  b = b / "..";  // out of project folder
  std::error_code ec;
  auto related = std::filesystem::relative(f, b, ec);
  return ec ? QString{} : QString::fromStdString(related.string());
}
