#include "newprojectmodel.h"

#include <QDir>

using namespace FOEDAG;

NewProjectModel::NewProjectModel(QObject *parent)
    : QObject{parent}, m_projectLocation(QDir::homePath()) {}

QString NewProjectModel::pageHeadCaption(const int index) {
  switch (index) {
    case 0:
      return QString(tr("Project Location"));
    case 1:
      return QString(tr("Project Type"));
    case 2:
      return QString(tr("Add Sources"));
    case 3:
      return QString(tr("Add Constraints (optional)"));
    case 4:
      return QString(tr("Device Planner"));
    case 5:
      return QString(tr("New Project Summary"));
    default:
      return QString(tr("Unknow page"));
  }
}

QString NewProjectModel::pageMainText(const int index) {
  switch (index) {
    case 0:
      return QString(tr(
          "This wizard will guide you through the creation of a new "
          "project.\r\n\r\n"
          "To create a Cwise project you will need to provide a name and a "
          "location for your project files. "
          "Next, you will specify the type of flow you'll be working with. "
          "Finally, you will specify your project sources and choose a default "
          "part. "));
    case 1:
      return QString(tr("Specify the type of project to create. "));
    case 2:
      return QString(
          tr("Specify HDL and IP files, or directories containing those files, "
             "to add to your project. Create a new source file on disk and add "
             "it to your project. "
             "You can also add and create source later. "));
    case 3:
      return QString(
          tr("Specify or create constraint file for physical and timing "
             "constraints. "));
    case 4:
      return QString(tr(
          "Select the series and device you want to target for compilation. "));
    case 5:
      return QString(tr("New Project Summary"));
    default:
      return QString(tr("Unknow page"));
  }
}

QString NewProjectModel::projectNameCaption() {
  return QString(tr("Project name:"));
}

QString NewProjectModel::projectLocationCaption() {
  return QString(tr("Project location:"));
}

QString NewProjectModel::checkBoxSubDirectoryCaption() {
  return QString(tr("Create project subdirectory"));
}

QString NewProjectModel::projectFullPathCaption() {
  return QString(tr("Project will be created at:"));
}

QString NewProjectModel::radioButtonRTLProjectCaption() {
  return QString(tr("RTL Project"));
}

QString NewProjectModel::textRTLProject() {
  return QString(
      tr("You will be able to add sources,create block designs in IP "
         "integrator, generate IP, "
         "run RTL analysis, synthesis, implementation, design planning and "
         "analysis. "));
}

QString NewProjectModel::radioButtonPostSynthesisProjectCaption() {
  return QString(tr("Gate-level Project"));
}

QString NewProjectModel::textPostSynthesisProject() {
  return QString(
      tr("You will be able to add sources, view device resources, run design "
         "analysis, planning and implementation. "));
}

QString NewProjectModel::fullPathToProject() {
  if (m_needToCreateProjrctSubDirectory && !m_projectName.isEmpty() &&
      !m_projectLocation.isEmpty()) {
    return m_projectLocation + "/" + m_projectName;
  } else {
    return m_projectLocation;
  }
}

const QString &NewProjectModel::projectName() const { return m_projectName; }

void NewProjectModel::setProjectName(const QString &newProjectName) {
  if (m_projectName == newProjectName) return;

  m_projectName = newProjectName;
  emit projectNameChanged();
}

const QString &NewProjectModel::projectLocation() const {
  return m_projectLocation;
}

void NewProjectModel::setProjectLocation(const QString &newProjectLocation) {
  if (m_projectLocation == newProjectLocation) return;

  m_projectLocation = newProjectLocation;
  emit projectLocationChanged();
}

bool NewProjectModel::needToCreateProjrctSubDirectory() const {
  return m_needToCreateProjrctSubDirectory;
}

void NewProjectModel::setNeedToCreateProjrctSubDirectory(
    bool newNeedToCreateProjrctSubDirectory) {
  if (m_needToCreateProjrctSubDirectory == newNeedToCreateProjrctSubDirectory)
    return;

  m_needToCreateProjrctSubDirectory = newNeedToCreateProjrctSubDirectory;
  emit needToCreateProjrctSubDirectoryChanged();
}

const QString &NewProjectModel::projectType() const { return m_projectType; }

void NewProjectModel::setProjectType(const QString &newProjectType) {
  if (m_projectType == newProjectType) return;

  m_projectType = newProjectType;
  emit projectTypeChanged();
}
