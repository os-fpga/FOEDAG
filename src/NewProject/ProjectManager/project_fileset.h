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

  void addFile(const QString &strName, const QString &strFilePath);
  QString getFilePath(const QString &strFileName);

  QString getSetName() const;
  void setSetName(const QString &setName);

  QString getSetType() const;
  void setSetType(const QString &setType);

  QString getRelSrcDir() const;
  void setRelSrcDir(const QString &relSrcDir);

  QMap<QString, QString> getMapFiles() const;

 private:
  QString m_setName;
  QString m_setType;
  QString m_relSrcDir;
  QMap<QString, QString> m_mapFiles;
};
}  // namespace FOEDAG
#endif  // PROJECTFILESET_H
