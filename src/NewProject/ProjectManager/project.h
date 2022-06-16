#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>

#include "project_configuration.h"
#include "project_fileset.h"
#include "project_run.h"

namespace FOEDAG {

class Project : public QObject {
  Q_OBJECT

 public:
  static Project *Instance();

  void InitProject();

  QString projectName() const;
  void setProjectName(const QString &projectName);

  QString projectPath() const;
  void setProjectPath(const QString &projectPath);

  ProjectConfiguration *projectConfig() const;

  ProjectFileSet *getProjectFileset(const QString &strName) const;
  int setProjectFileset(const ProjectFileSet &projectFileset);
  void deleteProjectFileset(const QString &strName);

  ProjectRun *getProjectRun(const QString &strName) const;
  int setProjectRun(const ProjectRun &projectRun);
  void deleteprojectRun(const QString &strName);

  QMap<QString, ProjectFileSet *> getMapProjectFileset() const;

  QMap<QString, ProjectRun *> getMapProjectRun() const;

 signals:
  void projectPathChanged();

 private:
  QString m_projectName;
  QString m_projectPath;

  ProjectConfiguration *m_projectConfig;
  QMap<QString, ProjectFileSet *> m_mapProjectFileset;
  QMap<QString, ProjectRun *> m_mapProjectRun;
};
}  // namespace FOEDAG
#endif  // PROJECT_H
