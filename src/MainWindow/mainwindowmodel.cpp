#include "mainwindowmodel.h"

#include "Console/StreamBuffer.h"
#include "Console/TclConsole.h"
#include "Console/TclConsoleBuilder.h"
#include "Console/TclConsoleWidget.h"
#include "DesignRuns/runs_form.h"
#include "Main/Foedag.h"
#include "NewFile/new_file.h"
#include "NewFile/newfilemodel.h"
#include "NewProject/new_project_dialog.h"
#include "NewProject/newprojectmodel.h"
#include "ProjNavigator/sources_form.h"
#include "TextEditor/text_editor.h"
using namespace FOEDAG;

MainWindowModel::MainWindowModel(TclInterpreter* interp, QObject* parent)
    : m_interpreter(interp),
      m_newFileModel(std::make_unique<NewFileModel>()),
      m_newProjectModel(std::make_unique<NewProjectModel>()) {
  connect(this, &MainWindowModel::projectNameChanged, m_newProjectModel.get(),
          &NewProjectModel::projectNameChanged);
  setStatusBarMessage(tr("Ready"));
}

void MainWindowModel::Tcl_NewProject(int argc, const char* argv[]) {
  ProjectManager* projectManager = new ProjectManager(this);
  projectManager->Tcl_CreateProject(argc, argv);
}

QStringList MainWindowModel::newFileDialogFilters() {
  return m_newFileModel->fileDialogFilters();
}

QStringList MainWindowModel::openFileDialogFilters() {
  return QStringList() << "FOEDAG Project File(*.ospr)";
}

QString MainWindowModel::pageHeadCaption(const int index) {
  return m_newProjectModel->pageHeadCaption(index);
}

QString MainWindowModel::pageMainText(const int index) {
  return m_newProjectModel->pageMainText(index);
}

QString MainWindowModel::projectNameCaption() {
  return m_newProjectModel->projectNameCaption();
}

QString MainWindowModel::projectLocationCaption() {
  return m_newProjectModel->projectLocationCaption();
}

QString MainWindowModel::checkBoxSubDirectoryCaption() {
  return m_newProjectModel->checkBoxSubDirectoryCaption();
}

QString MainWindowModel::projectFullPathCaption() {
  return m_newProjectModel->projectFullPathCaption();
}

QString MainWindowModel::radioButtonRTLProjectCaption() {
  return m_newProjectModel->radioButtonRTLProjectCaption();
}

QString MainWindowModel::textRTLProject() {
  return m_newProjectModel->textRTLProject();
}

QString MainWindowModel::radioButtonPostSynthesisProjectCaption() {
  return m_newProjectModel->radioButtonPostSynthesisProjectCaption();
}

QString MainWindowModel::textPostSynthesisProject() {
  return m_newProjectModel->textPostSynthesisProject();
}

QString MainWindowModel::radioButtonSynplifyProjectCaption() {
  return m_newProjectModel->radioButtonSynplifyProjectCaption();
}

QString MainWindowModel::textSynplifyProject() {
  return m_newProjectModel->textSynplifyProject();
}

QString MainWindowModel::fullPathToProject() {
  return m_newProjectModel->fullPathToProject();
}

const QString& MainWindowModel::projectName() const {
  return m_newProjectModel->projectName();
}

void MainWindowModel::setProjectName(const QString& newProjectName) {
  m_newProjectModel->setProjectName(newProjectName);
}

const QString& MainWindowModel::projectLocation() const {
  return m_newProjectModel->projectLocation();
}

void MainWindowModel::setProjectLocation(const QString& newProjectLocation) {
  m_newProjectModel->setProjectLocation(newProjectLocation);
}

bool MainWindowModel::needToCreateProjrctSubDirectory() const {
  return m_newProjectModel->needToCreateProjrctSubDirectory();
}

void MainWindowModel::setNeedToCreateProjrctSubDirectory(
    bool newNeedToCreateProjrctSubDirectory) {
  m_newProjectModel->setNeedToCreateProjrctSubDirectory(
      newNeedToCreateProjrctSubDirectory);
}

const QString& MainWindowModel::projectType() const {
  return m_newProjectModel->projectType();
}

void MainWindowModel::setProjectType(const QString& newProjectType) {
  m_newProjectModel->setProjectType(newProjectType);
}

bool MainWindowModel::isVisible() const { return m_isVisible; }

void MainWindowModel::setIsVisible(bool newIsVisible) {
  if (m_isVisible == newIsVisible) return;

  m_isVisible = newIsVisible;
  emit visibleChanged();
}

const QString& MainWindowModel::statusBarMessage() const {
  return m_statusBarMessage;
}

void MainWindowModel::setStatusBarMessage(const QString& newStatusBarMessage) {
  if (m_statusBarMessage == newStatusBarMessage) return;

  m_statusBarMessage = newStatusBarMessage;
  emit statusBarMessageChanged();
}

bool MainWindowModel::createNewFile(const QUrl& fileName,
                                    const QString& extension) {
  return m_newFileModel->createNewFileWithExtensionCheck(fileName.path(),
                                                         extension);
}
