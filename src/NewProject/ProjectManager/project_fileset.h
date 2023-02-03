#ifndef PROJECTFILESET_H
#define PROJECTFILESET_H
#include <QObject>

#include "project_option.h"

namespace FOEDAG {

struct CompilationUnit {
  int language;
  QString group;
};

class ProjectFileSet : public ProjectOption {
  Q_OBJECT
 public:
  explicit ProjectFileSet(QObject *parent = nullptr);

  ProjectFileSet &operator=(const ProjectFileSet &other);
  QString getDefaultUnitName() const;

  void addFile(const QString &strFileName, const QString &strFilePath);
  void addFiles(const QStringList &commands, const QStringList &libs,
                const QStringList &files, int language, const QString &gr);
  QString getFilePath(const QString &strFileName);
  void deleteFile(const QString &strFileName);

  QString getSetName() const;
  void setSetName(const QString &setName);

  QString getSetType() const;
  void setSetType(const QString &setType);

  QString getRelSrcDir() const;
  void setRelSrcDir(const QString &relSrcDir);

  const std::vector<std::pair<QString, QString>> &getMapFiles() const;
  const std::vector<std::pair<CompilationUnit, QStringList>> &Files() const;

  const std::vector<std::pair<QStringList, QStringList>> &getLibraries() const;

 private:
  QString m_setName;
  QString m_setType;
  QString m_relSrcDir;
  std::vector<std::pair<QString, QString>> m_mapFiles;
  std::vector<std::pair<CompilationUnit, QStringList>> m_langMap;
  std::vector<std::pair<QStringList, QStringList>>
      m_commandsLibs;  // Collection of commands with corresponding libraries.
                       // Synchronized with m_langMap.
};
}  // namespace FOEDAG
#endif  // PROJECTFILESET_H
