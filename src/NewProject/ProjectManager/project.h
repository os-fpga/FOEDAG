#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>

#include "project_configuration.h"
#include "project_fileset.h"
#include "project_run.h"

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

  ProjectFileSet *getProjectFileset(const QString &strname) const;
  void setProjectFileset(ProjectFileSet *projectFileset);

  ProjectRun *getProjectRun(const QString &strname) const;
  void setProjectRun(ProjectRun *projectRun);

  QMap<QString, ProjectFileSet *> getMapProjectFileset() const;

  QMap<QString, ProjectRun *> getMapProjectRun() const;

 signals:

 private:
  QString m_projectName;
  QString m_projectPath;

  ProjectConfiguration *m_projectConfig;
  QMap<QString, ProjectFileSet *> m_mapProjectFileset;
  QMap<QString, ProjectRun *> m_mapProjectRun;
};

#endif  // PROJECT_H
