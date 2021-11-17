#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H
#include <QObject>

#include "project_option.h"

class ProjectConfiguration : public ProjectOption {
  Q_OBJECT
 public:
  explicit ProjectConfiguration(QObject *parent = nullptr);

  QString id() const;
  void setId(const QString &id);

  QString projectType() const;
  void setProjectType(const QString &projectType);

  QString activeSimSet() const;
  void setActiveSimSet(const QString &activeSimSet);

  QString simulationTopMoule() const;
  void setSimulationTopMoule(const QString &simulationTopMoule);

 private:
  QString m_id;
  QString m_projectType;
  QString m_activeSimSet;
  QString m_simulationTopMoule;

  void initProjectID();
};

#endif  // PROJECTCONFIG_H
