#include "new_project_dialog.h"

#include <QDebug>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QProxyStyle>
#include <QThread>

#include "Compiler/Compiler.h"
#include "MainWindow/Session.h"
#include "ui_new_project_dialog.h"

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
  setWindowTitle(tr("New Project"));
  BackBtn = new QPushButton("Back", this);
  ui->buttonBox->addButton(BackBtn, QDialogButtonBox::ButtonRole::ActionRole);
  connect(BackBtn, &QPushButton::clicked, this, &newProjectDialog::on_back);

  NextBtn = new QPushButton("Next", this);
  ui->buttonBox->addButton(NextBtn, QDialogButtonBox::ButtonRole::ActionRole);
  connect(NextBtn, &QPushButton::clicked, this, &newProjectDialog::on_next);

  ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Finish");
  ui->m_tabWidget->tabBar()->setStyle(new CustomTabStyle);
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

void newProjectDialog::Reset() {
  m_index = INDEX_LOCATION;
  m_skipSources = false;

  ui->m_tabWidget->clear();

  m_locationForm = new locationForm(this);
  ui->m_tabWidget->insertTab(INDEX_LOCATION, m_locationForm,
                             tr("Project Directory"));
  m_proTypeForm = new projectTypeForm(this);
  ui->m_tabWidget->insertTab(INDEX_PROJTYPE, m_proTypeForm,
                             tr("Type of Project"));
  QObject::connect(m_proTypeForm, &projectTypeForm::skipSources,
                   [this](bool skip) { m_skipSources = skip; });
  m_addSrcForm = new addSourceForm(this);
  ui->m_tabWidget->insertTab(INDEX_ADDSOURC, m_addSrcForm,
                             tr("Add Design Files"));
  m_addConstrsForm = new addConstraintsForm(this);
  ui->m_tabWidget->insertTab(INDEX_ADDCONST, m_addConstrsForm,
                             tr("Add Design Constraints"));
  m_devicePlanForm = new devicePlannerForm(this);
  ui->m_tabWidget->insertTab(INDEX_DEVICEPL, m_devicePlanForm,
                             tr("Select Target Device"));
  m_sumForm = new summaryForm(this);
  ui->m_tabWidget->insertTab(INDEX_SUMMARYF, m_sumForm, tr("Summary"));
  ui->m_tabWidget->adjustSize();
  // Disable tab selection (by mouse and keyboard)
  ui->m_tabWidget->tabBar()->setAttribute(Qt::WA_TransparentForMouseEvents);
  ui->m_tabWidget->tabBar()->setFocusPolicy(Qt::NoFocus);

  UpdateDialogView();
}

void newProjectDialog::UpdateDialogView() {
  if (INDEX_LOCATION == m_index) {
    BackBtn->setEnabled(false);
  } else {
    BackBtn->setEnabled(true);
  }

  if (INDEX_SUMMARYF == m_index) {
    NextBtn->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_sumForm->setProjectName(m_locationForm->getProjectName(),
                              m_proTypeForm->getProjectType());
    m_sumForm->setDeviceInfo(m_devicePlanForm->getSelectedDevice());
    m_sumForm->setSourceCount(m_addSrcForm->getFileData().count(),
                              m_addConstrsForm->getFileData().count());
  } else {
    NextBtn->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  }

  ui->m_tabWidget->setCurrentIndex(m_index);
}

void newProjectDialog::on_buttonBox_accepted() {
  ProjectOptions opt{
      m_locationForm->getProjectName(),
      m_locationForm->getProjectPath(),
      m_proTypeForm->getProjectType(),
      {m_addSrcForm->getFileData(), m_addSrcForm->IsCopySource()},
      {m_addConstrsForm->getFileData(), m_addConstrsForm->IsCopySource()},
      m_devicePlanForm->getSelectedDevice(),
      false /*rewrite*/,
      DEFAULT_FOLDER_SOURCE,
      m_addSrcForm->TopModule(),
      m_addSrcForm->LibraryForTopModule()};
  Compiler *compiler = GlobalSession->GetCompiler();
  if (m_addConstrsForm->IsRandom()) {
    compiler->PinAssignOpts(Compiler::PinAssignOpt::Random);
  } else
    compiler->PinAssignOpts(Compiler::PinAssignOpt::In_Define_Order);
  m_projectManager->CreateProject(opt);
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
  }
  if (m_skipSources && m_index == INDEX_PROJTYPE)
    m_index += 3;  // omit design and constraint files
  else
    m_index++;
  UpdateDialogView();
}

void newProjectDialog::on_back() {
  if (m_skipSources && m_index == INDEX_DEVICEPL)
    m_index -= 3;  // omit design and constraint files
  else
    m_index--;
  UpdateDialogView();
}
