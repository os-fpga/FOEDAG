#ifndef PROJECTFILESET_H
#define PROJECTFILESET_H
#include <QObject>

#include "project_option.h"

namespace FOEDAG {

class ProjectFileSet : public ProjectOption {
  Q_OBJECT
 public:
  explicit ProjectFileSet(QObject *parent = nullptr);

  ProjectFileSet &operator=(const ProjectFileSet &other);

  void addFile(const QString &strFileName, const QString &strFilePath);
  void addFiles(const QStringList &files, int language);
  QString getFilePath(const QString &strFileName);
  void deleteFile(const QString &strFileName);

  QString getSetName() const;
  void setSetName(const QString &setName);

  QString getSetType() const;
  void setSetType(const QString &setType);

  QString getRelSrcDir() const;
  void setRelSrcDir(const QString &relSrcDir);

  const std::vector<std::pair<QString, QString>> &getMapFiles() const;
  const std::vector<std::pair<int, QStringList>> &Files() const;

 private:
  QString m_setName;
  QString m_setType;
  QString m_relSrcDir;
  std::vector<std::pair<QString, QString>> m_mapFiles;
  std::vector<std::pair<int, QStringList>> m_langMap;
};
}  // namespace FOEDAG
#endif  // PROJECTFILESET_H
