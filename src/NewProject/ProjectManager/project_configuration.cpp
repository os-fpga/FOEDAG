#include "project_configuration.h"

#include <QTime>

ProjectConfiguration::ProjectConfiguration(QObject *parent)
    : ProjectOption(parent) {
  initProjectID();
}

QString ProjectConfiguration::id() const { return m_id; }

QString ProjectConfiguration::projectType() const { return m_projectType; }

void ProjectConfiguration::setProjectType(const QString &projectType) {
  m_projectType = projectType;
}

QString ProjectConfiguration::activeSimSet() const { return m_activeSimSet; }

void ProjectConfiguration::setActiveSimSet(const QString &activeSimSet) {
  m_activeSimSet = activeSimSet;
}

QString ProjectConfiguration::simulationTopMoule() const {
  return m_simulationTopMoule;
}

void ProjectConfiguration::setSimulationTopMoule(
    const QString &simulationTopMoule) {
  m_simulationTopMoule = simulationTopMoule;
}

void ProjectConfiguration::initProjectID() {
  QDateTime curDateTime = QDateTime::currentDateTime();

  m_id = m_id.sprintf("%04d%02d%02d%02d%02d%02d%03d", curDateTime.date().year(),
                      curDateTime.date().month(), curDateTime.date().day(),
                      curDateTime.time().hour(), curDateTime.time().minute(),
                      curDateTime.time().second(), curDateTime.time().msec());
}
