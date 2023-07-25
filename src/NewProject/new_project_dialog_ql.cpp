#include "new_project_dialog.h"

#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QProxyStyle>
#include <QThread>

#include "Compiler/Compiler.h"
#include "MainWindow/Session.h"
#include "ui_new_project_dialog.h"
#include "Compiler/QLSettingsManager.h"
#include "Compiler/QLDeviceManager.h"

extern FOEDAG::Session *GlobalSession;
using namespace FOEDAG;

namespace {
// Custom style to get horizontally shown tab labels in QTabWidget
class CustomTabStyle : public QProxyStyle {
 public:
  QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                         const QSize &size,
                         const QWidget *widget) const override {
    auto s = QProxyStyle::sizeFromContents(type, option, size, widget);
    if (type == QStyle::CT_TabBarTab) {
      s.transpose();
    }
    return s;
  }

  void drawControl(ControlElement element, const QStyleOption *option,
                   QPainter *painter, const QWidget *widget) const override {
    if (element == CE_TabBarTabLabel) {
      if (const QStyleOptionTab *tab =
              qstyleoption_cast<const QStyleOptionTab *>(option)) {
        QStyleOptionTab opt(*tab);
        opt.shape = QTabBar::RoundedNorth;
        QProxyStyle::drawControl(element, &opt, painter, widget);
        return;
      }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
  }
  // By default, tab label is centered. In the current method we align it left
  void drawItemText(
      QPainter *painter, const QRect &rect, int flags, const QPalette &pal,
      bool enabled, const QString &text,
      QPalette::ColorRole textRole = QPalette::NoRole) const override {
    return QProxyStyle::drawItemText(painter, rect,
                                     Qt::AlignLeft | Qt::AlignVCenter, pal,
                                     enabled, text, textRole);
  }
};
}  // namespace

newProjectDialog::newProjectDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::newProjectDialog), m_index(INDEX_LOCATION) {
  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  BackBtn = new QPushButton("Back", this);
  ui->buttonBox->addButton(BackBtn, QDialogButtonBox::ButtonRole::ActionRole);
  connect(BackBtn, &QPushButton::clicked, this, &newProjectDialog::on_back);

  NextBtn = new QPushButton("Next", this);
  ui->buttonBox->addButton(NextBtn, QDialogButtonBox::ButtonRole::ActionRole);
  connect(NextBtn, &QPushButton::clicked, this, &newProjectDialog::on_next);

  ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Finish");
  ui->m_tabWidget->tabBar()->setStyle(new CustomTabStyle);
  connect(ui->m_tabWidget, &QTabWidget::currentChanged, this,
          &newProjectDialog::updateSummaryPage);
  Reset();

  m_projectManager = new ProjectManager(this);
}

newProjectDialog::~newProjectDialog() { delete ui; }

void newProjectDialog::Next_TclCommand_Test() {
  QThread::sleep(0);
  emit NextBtn->clicked();
}

void newProjectDialog::CreateProject_Tcl_Test(int argc, const char *argv[]) {
  m_projectManager->Tcl_CreateProject(argc, argv);
}

QString newProjectDialog::getProject() {
  return m_locationForm->getProjectPath() + "/" +
         m_locationForm->getProjectName() + PROJECT_FILE_FORMAT;
}

void newProjectDialog::Reset(Mode mode) {
  m_mode = mode;
  m_index = INDEX_LOCATION;
  m_skipSources = false;
  QVector<QWidget *> oldWidgets;
  for (int i = 0; i < ui->m_tabWidget->count(); i++)
    oldWidgets.push_back(ui->m_tabWidget->widget(i));
  ui->m_tabWidget->clear();

  if (mode == NewProject)
    ResetToNewProject();
  else
    ResetToProjectSettings();

  UpdateDialogView(mode);
  qDeleteAll(oldWidgets);
}

Mode newProjectDialog::GetMode() const { return m_mode; }

void newProjectDialog::SetPageActive(FormIndex index) {
  if (m_tabIndexes.contains(index)) {
    ui->m_tabWidget->setCurrentIndex(m_tabIndexes.value(index));
  } else {
    ui->m_tabWidget->setCurrentIndex(static_cast<int>(index));
  }
}

void newProjectDialog::SetDefaultPath(const QString &path) {
  m_defaultPath = path;
}

void newProjectDialog::updateSummaryPage() {
  if (m_mode == Mode::NewProject) return;
  auto currentPage = ui->m_tabWidget->currentWidget();
  if (currentPage == m_sumForm) {
    updateSummary(
        m_projectManager->getProjectName(),
        projectTypeForm::projectTypeStr(m_projectManager->projectType()));
  }
}

void newProjectDialog::UpdateDialogView(Mode mode) {
  if (INDEX_LOCATION == m_index) {
    BackBtn->setEnabled(false);
  } else {
    BackBtn->setEnabled(true);
  }

  if (INDEX_SUMMARYF == m_index) {
    NextBtn->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    updateSummary(m_locationForm->getProjectName(),
                  m_proTypeForm->projectTypeStr());
  } else {
    NextBtn->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  }

  ui->m_tabWidget->setCurrentIndex(m_index);
  if (mode == ProjectSettings)
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void newProjectDialog::ResetToNewProject() {
  setWindowTitle(tr("New Project"));

  m_locationForm = new locationForm(m_defaultPath, this);
  ui->m_tabWidget->insertTab(INDEX_LOCATION, m_locationForm,
                             tr("Project Directory"));
  
  m_proTypeForm = new projectTypeForm(this);
  ui->m_tabWidget->insertTab(INDEX_PROJTYPE, m_proTypeForm,
                             tr("Type of Project"));
  QObject::connect(m_proTypeForm, &projectTypeForm::skipSources, this,
                   [this](bool skip) { m_skipSources = skip; });
  
  m_addSrcForm = new addSourceForm(GT_SOURCE, this);
  m_addSrcForm->SetTitle("Add Design Files");
  ui->m_tabWidget->insertTab(INDEX_ADDSOURC, m_addSrcForm,
                             tr("Add Design Files"));
  
  m_addSimForm = new addSourceForm(GT_SIM, this);
  m_addSimForm->SetTitle("Add Simulation Files");
  // KK: remove Simulation Files Form (not using yet)
  // ui->m_tabWidget->insertTab(INDEX_ADDSIM, m_addSimForm,
  //                            tr("Add Simulation Files"));
  m_addSimForm->setVisible(false);
  
  m_addConstrsForm = new addConstraintsForm(this);
  m_addConstrsForm->SetTitle("Add Design Constraints (optional)");
  // KK: remove Constraint File Form (use JSON/Settings GUI only)
  // ui->m_tabWidget->insertTab(INDEX_ADDCONST, m_addConstrsForm,
  //                            tr("Add Design Constraints"));
  m_addConstrsForm->setVisible(false);

  // KK: remove Device Planner Form, we use our own Device Selection Widget
  // m_devicePlanForm = new devicePlannerForm(this);
  // ui->m_tabWidget->insertTab(INDEX_DEVICEPL, m_devicePlanForm,
  //                            tr("Select Target Device"));

  QWidget* m_QLDeviceSelectionWidget = 
    QLDeviceManager::getInstance()->createDeviceSelectionWidget(true);
  ui->m_tabWidget->insertTab(INDEX_DEVICEPL, m_QLDeviceSelectionWidget,
                             tr("Target Device"));

  // ensure that we instantiate the QLSettingsManager
  QWidget* m_QLSettingsWidget = QLSettingsManager::getInstance()->createSettingsWidget(true);
  ui->m_tabWidget->insertTab(INDEX_QLSETTIN, m_QLSettingsWidget,
                             tr("Task Settings"));

  m_sumForm = new summaryForm(this);
  ui->m_tabWidget->insertTab(INDEX_SUMMARYF, m_sumForm, tr("Summary"));

  ui->m_tabWidget->adjustSize();
  // Disable tab selection (by mouse and keyboard)
  ui->m_tabWidget->tabBar()->setAttribute(Qt::WA_TransparentForMouseEvents);
  ui->m_tabWidget->tabBar()->setFocusPolicy(Qt::NoFocus);
  BackBtn->setVisible(true);
  NextBtn->setVisible(true);
}

void newProjectDialog::ResetToProjectSettings() {
  setWindowTitle(tr("Project settings"));
  m_settings.clear();
  m_locationForm = nullptr;
  m_proTypeForm = nullptr;

  m_addSrcForm = new addSourceForm(GT_SOURCE, this);
  m_addSrcForm->SetTitle("Design Files");
  m_settings.append(m_addSrcForm);
  auto index = ui->m_tabWidget->insertTab(INDEX_ADDSOURC, m_addSrcForm,
                                          tr("Design Files"));
  m_tabIndexes.insert(INDEX_ADDSOURC, index);

  m_addSimForm = new addSourceForm(GT_SIM, this);
  m_addSimForm->SetTitle("Simulation Files");
  // KK: remove Simulation Files Form (not using yet)
  // m_settings.append(m_addSimForm);
  // index = ui->m_tabWidget->insertTab(INDEX_ADDSIM, m_addSimForm,
  //                                    tr("Simulation Files"));
  // m_tabIndexes.insert(INDEX_ADDSIM, index);
  m_addSimForm->setVisible(false);

  m_addConstrsForm = new addConstraintsForm(this);
  m_addConstrsForm->SetTitle("Design Constraints");
  // KK: remove Constraint File Form (use JSON/Settings GUI only)
  // m_settings.append(m_addConstrsForm);
  // index = ui->m_tabWidget->insertTab(INDEX_ADDCONST, m_addConstrsForm,
  //                                    tr("Design Constraints"));
  // m_tabIndexes.insert(INDEX_ADDCONST, index);
  m_addConstrsForm->setVisible(false);

  // KK: remove Device Planner Form, we use our own Device Selection Widget
  // m_devicePlanForm = new devicePlannerForm(this);
  // m_settings.append(m_devicePlanForm);
  // index = ui->m_tabWidget->insertTab(INDEX_DEVICEPL, m_devicePlanForm,
  //                                    tr("Select Target Device"));
  // m_tabIndexes.insert(INDEX_DEVICEPL, index);

  QWidget* m_QLDeviceSelectionWidget = 
    QLDeviceManager::getInstance()->createDeviceSelectionWidget(false);
  ui->m_tabWidget->insertTab(INDEX_DEVICEPL, m_QLDeviceSelectionWidget,
                             tr("Target Device"));
  m_tabIndexes.insert(INDEX_DEVICEPL, index);

  // ensure that we instantiate the QLSettingsManager
  QWidget* m_QLSettingsWidget = QLSettingsManager::getInstance()->createSettingsWidget(false);
  ui->m_tabWidget->insertTab(INDEX_QLSETTIN, m_QLSettingsWidget,
                             tr("Task Settings"));
  m_tabIndexes.insert(INDEX_QLSETTIN, index);

  m_sumForm = new summaryForm(this);
  m_sumForm->setProjectSettings(true);
  index = ui->m_tabWidget->insertTab(INDEX_SUMMARYF, m_sumForm, tr("Summary"));
  m_tabIndexes.insert(INDEX_SUMMARYF, index);

  for (auto &settings : m_settings) settings->updateUi(m_projectManager);

  ui->m_tabWidget->adjustSize();
  // Disable tab selection (by mouse and keyboard)
  ui->m_tabWidget->tabBar()->setAttribute(Qt::WA_TransparentForMouseEvents,
                                          false);
  ui->m_tabWidget->tabBar()->setFocusPolicy(Qt::NoFocus);
  BackBtn->setVisible(false);
  NextBtn->setVisible(false);
  m_addSrcForm->SetBasePath(m_projectManager->getProjectPath());
  m_addSrcForm->setProjectType(m_projectManager->projectType());
}

// This will check the project wizard for potential conflicts
// it will return an std::pair of a bool representing the validity of the
// current values and a QString with any issues found. This function is intended
// to grow as more potential conflicts are discovered.
std::pair<bool, QString> newProjectDialog::ValuesValid() const {
  QString errStr{};

  // Check for Language type mistmatches in Compile Units
  auto conflictKeys = FindCompileUnitConflicts();
  if (!conflictKeys.isEmpty()) {
    errStr += "The following Compile Units have a language mismatch: " +
              conflictKeys.join(", ") +
              "\nReturn to the Design Files tab to fix this.";
  }

  return std::make_pair(errStr.isEmpty(), errStr);
}

QList<QString> newProjectDialog::FindCompileUnitConflicts() const {
  // Group files by Compile Unit (m_groupName)
  QMap<QString, QList<filedata>> fileGroups{};
  for (const filedata &fdata : m_addSrcForm->getFileData()) {
    if (!fdata.m_groupName.isEmpty()) {
      fileGroups[fdata.m_groupName].append(fdata);
    }
  }

  // Determine if any group has a language mismatch
  QStringList conflictKeys{};
  // Step through keys
  for (auto key : fileGroups.keys()) {
    int lang = -1;
    // Step through files in this group
    for (auto file : fileGroups[key]) {
      if (lang == -1) {
        lang = file.m_language;
      } else {
        // track any groups with a mismatch
        if (lang != file.m_language) {
          conflictKeys.append(key);
          break;
        }
      }
    }
  }

  return conflictKeys;
}

void newProjectDialog::updateSummary(const QString &projectName,
                                     const QString &projectType) {
  m_sumForm->setProjectName(projectName, projectType);
//   m_sumForm->setDeviceInfo(m_devicePlanForm->getSelectedDevice());
  m_sumForm->setSourceCount(m_addSrcForm->getFileData().count(),
                            m_addConstrsForm->getFileData().count(),
                            m_addSimForm->getFileData().count());
}

void newProjectDialog::on_buttonBox_accepted() {
  auto [isValid, err] = ValuesValid();
  if (!isValid) {
    QMessageBox::warning(this, "Project Settings Issue", err);
    return;
  }

  ProjectOptions opt{
      m_locationForm ? m_locationForm->getProjectName()
                     : m_projectManager->getProjectName(),
      m_locationForm ? m_locationForm->getProjectPath()
                     : m_projectManager->getProjectPath(),
      m_proTypeForm ? m_proTypeForm->projectType()
                    : m_projectManager->projectType(),
      {m_addSrcForm->getFileData(), m_addSrcForm->IsCopySource()},
      {m_addConstrsForm->getFileData(), m_addConstrsForm->IsCopySource()},
      {m_addSimForm->getFileData(), m_addSimForm->IsCopySource()},
      /*m_devicePlanForm->getSelectedDevice()*/QStringList(),
      false /*rewrite*/,
      DEFAULT_FOLDER_SOURCE,
      ProjectOptions::Options{
          m_addSrcForm->TopModule(), m_addSrcForm->LibraryForTopModule(),
          m_addSrcForm->IncludePath(), m_addSrcForm->LibraryPath(),
          m_addSrcForm->LibraryExt(), m_addSrcForm->Macros()},
      ProjectOptions::Options{
          m_addSimForm->TopModule(), m_addSimForm->LibraryForTopModule(),
          m_addSimForm->IncludePath(), m_addSimForm->LibraryPath(),
          m_addSimForm->LibraryExt(), m_addSimForm->Macros()}};
  Compiler *compiler = GlobalSession->GetCompiler();
  if (m_addConstrsForm->IsRandom())
    compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
  else if (m_addConstrsForm->IsFree())
    compiler->PinAssignOpts(Compiler::PinAssignOpt::Free);
  else
    compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
  if (m_mode == NewProject) {
    m_projectManager->CreateProject(opt);
  } else {
    m_projectManager->UpdateProject(opt);
  }
  accept();
}

void newProjectDialog::on_buttonBox_rejected() { reject(); }

void newProjectDialog::on_next() {
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
          tr("Project name already exists, Please rename for your project"),
          QMessageBox::Ok);
      return;
    }
    m_addSrcForm->SetBasePath(m_locationForm->getProjectPath());
  } else if (INDEX_PROJTYPE == m_index) {
    auto projectType = m_addSrcForm->projectType();
    int filesCount = m_addSrcForm->getFileData().count();
    if (projectType != NO_PROJECT_TYPE &&
        projectType != m_proTypeForm->projectType() && filesCount != 0) {
      auto answer =
          QMessageBox::question(this, "Project type changed",
                                "Project type has changed. Design source files "
                                "will be removed. Do you want to continue?");
      if (answer == QMessageBox::No) return;
      m_addSrcForm->clear();
    }
    m_addSrcForm->setProjectType(m_proTypeForm->projectType());
  }
  if (m_skipSources && m_index == INDEX_PROJTYPE)
    m_index = INDEX_DEVICEPL;  // omit design and constraint files
  else
    m_index++;
  UpdateDialogView();
}

void newProjectDialog::on_back() {
//   if (m_skipSources && m_index == INDEX_DEVICEPL)
//     m_index = INDEX_PROJTYPE;  // omit design and constraint files
//   else
    m_index--;
  UpdateDialogView();
}
