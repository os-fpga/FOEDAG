#include "project_fileset.h"
using namespace FOEDAG;

#define PROJECT_OSRCDIR "$OSRCDIR"

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
  this->m_langMap = other.m_langMap;
  this->m_commandsLibs = other.m_commandsLibs;
  ProjectOption::operator=(other);

  return *this;
}

QString ProjectFileSet::getDefaultUnitName() const {
  uint counter{0};
  const QString base{"unit_"};
  for (const auto &group : m_langMap) {
    auto name{QString{"%1%2"}.arg(base, QString::number(counter))};
    if (group.first.group == name)
      counter++;
    else
      return name;
  }
  return QString{"%1%2"}.arg(base, QString::number(counter));
}

void ProjectFileSet::addFile(const QString &strFileName,
                             const QString &strFilePath) {
  m_mapFiles.push_back(std::make_pair(strFileName, strFilePath));
}

void ProjectFileSet::addFiles(const QStringList &commands,
                              const QStringList &libs, const QStringList &files,
                              int language, const QString &gr) {
  m_langMap.push_back(std::make_pair(CompilationUnit{language, gr}, files));
  m_commandsLibs.push_back(std::make_pair(commands, libs));
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
    const QString file = iter->second;
    m_mapFiles.erase(iter);

    auto searchPath = [](const QStringList &source,
                         const QString &searchStr) -> int {
      auto tmp = searchStr;
      tmp.replace(PROJECT_OSRCDIR, QString{});
      for (int i = 0; i < source.size(); i++) {
        if (source.at(i).endsWith(tmp)) return i;
      }
      return -1;
    };

    for (auto it = m_langMap.begin(); it != m_langMap.end(); ++it) {
      auto index = searchPath(it->second, file);
      if (index != -1) {
        it->second.removeAt(index);
        if (it->second.isEmpty()) {
          auto dst = std::distance(m_langMap.begin(), it);
          m_langMap.erase(it);
          m_commandsLibs.erase(m_commandsLibs.begin() + dst);
        }
        break;
      }
    }
  }
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

const std::vector<std::pair<CompilationUnit, QStringList>>
    &ProjectFileSet::Files() const {
  return m_langMap;
}

const std::vector<std::pair<QStringList, QStringList>>
    &ProjectFileSet::getLibraries() const {
  return m_commandsLibs;
}
