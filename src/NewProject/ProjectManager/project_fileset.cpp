#include "project_fileset.h"

ProjectFileSet::ProjectFileSet(QObject *parent) : ProjectOption(parent) {}

ProjectFileSet &ProjectFileSet::operator=(const ProjectFileSet &other) {
  this->m_setName = other.m_setName;
  this->m_setType = other.m_setType;
  this->m_relSrcDir = other.m_relSrcDir;
  this->m_mapFiles = other.m_mapFiles;
  return *this;
}

void ProjectFileSet::addFile(const QString &strName,
                             const QString &strFilePath) {
  m_mapFiles[strName] = strFilePath;
}

QString ProjectFileSet::getFilePath(const QString &strFileName) {
  QString retStr = "";
  auto iter = m_mapFiles.find(strFileName);
  if (iter != m_mapFiles.end()) {
    retStr = iter.value();
  }
  return retStr;
}

QString ProjectFileSet::getSetName() const { return m_setName; }

void ProjectFileSet::setSetName(const QString &setName) { m_setName = setName; }

QString ProjectFileSet::getSetType() const { return m_setType; }

void ProjectFileSet::setSetType(const QString &setType) { m_setType = setType; }

QString ProjectFileSet::getRelSrcDir() const { return m_relSrcDir; }

void ProjectFileSet::setRelSrcDir(const QString &relSrcDir) {
  m_relSrcDir = relSrcDir;
}
