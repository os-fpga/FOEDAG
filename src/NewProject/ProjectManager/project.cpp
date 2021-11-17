#include "project.h"

Q_GLOBAL_STATIC(Project, project)

Project *Project::Instance() { return project(); }

void Project::InitProject() {
  m_projectName = "";
  m_projectPath = "";
  m_projectConfig = new ProjectConfiguration(this);
  m_mapProjectRun.clear();
  m_mapProjectFileset.clear();
}

QString Project::projectName() const { return m_projectName; }

void Project::setProjectName(const QString &projectName) {
  m_projectName = projectName;
}

QString Project::projectPath() const { return m_projectPath; }

void Project::setProjectPath(const QString &projectPath) {
  m_projectPath = projectPath;
}

ProjectConfiguration *Project::projectConfig() const { return m_projectConfig; }

ProjectFileSet *Project::getProjectFileset(const QString &strname) const {
  ProjectFileSet *retProFileSet = nullptr;
  auto iter = m_mapProjectFileset.find(strname);
  if (iter != m_mapProjectFileset.end()) {
    retProFileSet = iter.value();
  }
  return retProFileSet;
}

void Project::setProjectFileset(ProjectFileSet *projectFileset) {
  if (nullptr != projectFileset) {
    m_mapProjectFileset.insert(projectFileset->getSetName(), projectFileset);
  }
}

ProjectRun *Project::getProjectRun(const QString &strname) const {
  ProjectRun *retProRun = nullptr;
  auto iter = m_mapProjectRun.find(strname);
  if (iter != m_mapProjectRun.end()) {
    retProRun = iter.value();
  }
  return retProRun;
}

void Project::setProjectRun(ProjectRun *projectRun) {
  if (nullptr != projectRun) {
    m_mapProjectRun.insert(projectRun->runName(), projectRun);
  }
}

QMap<QString, ProjectFileSet *> Project::getMapProjectFileset() const {
  return m_mapProjectFileset;
}

QMap<QString, ProjectRun *> Project::getMapProjectRun() const {
  return m_mapProjectRun;
}
