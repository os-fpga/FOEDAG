#include "project_fileset.h"
using namespace FOEDAG;

ProjectFileSet::ProjectFileSet(QObject *parent) : ProjectOption(parent) {
  m_setName = "";
  m_setType = "";
  m_relSrcDir = "";
  m_mapFiles.clear();
}

ProjectFileSet &ProjectFileSet::operator=(const ProjectFileSet &other) {
  if (this == &other) {
    return *this;
  }
  this->m_setName = other.m_setName;
  this->m_setType = other.m_setType;
  this->m_relSrcDir = other.m_relSrcDir;
  this->m_mapFiles = other.m_mapFiles;
  ProjectOption::operator=(other);

  return *this;
}

void ProjectFileSet::addFile(const QString &strFileName,
                             const QString &strFilePath) {
  m_mapFiles.push_back(std::make_pair(strFileName, strFilePath));
}

void ProjectFileSet::addFiles(const QStringList &files, int language) {
  m_langMap.push_back(std::make_pair(language, files));
}

QString ProjectFileSet::getFilePath(const QString &strFileName) {
  QString retStr;
  auto iter = std::find_if(m_mapFiles.cbegin(), m_mapFiles.cend(),
                           [&](const std::pair<QString, QString> &p) {
                             return p.first == strFileName;
                           });
  if (iter != m_mapFiles.end()) {
    retStr = iter->second;
  }
  return retStr;
}

void ProjectFileSet::deleteFile(const QString &strFileName) {
  auto iter = std::find_if(m_mapFiles.cbegin(), m_mapFiles.cend(),
                           [&](const std::pair<QString, QString> &p) {
                             return p.first == strFileName;
                           });
  if (iter != m_mapFiles.end()) {
    m_mapFiles.erase(iter);
  }
  // TODO delete from m_langMap
}

QString ProjectFileSet::getSetName() const { return m_setName; }

void ProjectFileSet::setSetName(const QString &setName) { m_setName = setName; }

QString ProjectFileSet::getSetType() const { return m_setType; }

void ProjectFileSet::setSetType(const QString &setType) { m_setType = setType; }

QString ProjectFileSet::getRelSrcDir() const { return m_relSrcDir; }

void ProjectFileSet::setRelSrcDir(const QString &relSrcDir) {
  m_relSrcDir = relSrcDir;
}

const std::vector<std::pair<QString, QString>> &ProjectFileSet::getMapFiles()
    const {
  return m_mapFiles;
}

const std::vector<std::pair<int, QStringList>> &ProjectFileSet::Files() const {
  return m_langMap;
}
