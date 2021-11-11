#ifndef PROJECTRUN_H
#define PROJECTRUN_H
#include <QObject>

#include "project_option.h"

class ProjectRun : public ProjectOption {
  Q_OBJECT
 public:
  explicit ProjectRun(QObject *parent = nullptr);

  ProjectRun &operator=(const ProjectRun &other);

  QString runName() const;
  void setRunName(const QString &runName);

  QString runType() const;
  void setRunType(const QString &runType);

  QString srcSet() const;
  void setSrcSet(const QString &srcSet);

  QString constrsSet() const;
  void setConstrsSet(const QString &constrsSet);

  QString runState() const;
  void setRunState(const QString &runState);

  QString synthRun() const;
  void setSynthRun(const QString &synthRun);

 private:
  QString m_runName;
  QString m_runType;
  QString m_srcSet;
  QString m_constrsSet;
  QString m_runState;
  QString m_synthRun;
};

#endif  // PROJECTRUN_H
