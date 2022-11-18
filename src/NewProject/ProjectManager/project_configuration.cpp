#include "project_configuration.h"

#include <QTime>

using namespace FOEDAG;

ProjectConfiguration::ProjectConfiguration(QObject *parent)
    : ProjectOption(parent) {
  initProjectID();
  m_projectType = 0;  // RTL
  m_activeSimSet = "";
}

QString ProjectConfiguration::id() const { return m_id; }

void ProjectConfiguration::setId(const QString &id) { m_id = id; }

int ProjectConfiguration::projectType() const { return m_projectType; }

void ProjectConfiguration::setProjectType(int projectType) {
  m_projectType = projectType;
}

QString ProjectConfiguration::activeSimSet() const { return m_activeSimSet; }

void ProjectConfiguration::setActiveSimSet(const QString &activeSimSet) {
  m_activeSimSet = activeSimSet;
}

void ProjectConfiguration::initProjectID() {
  QDateTime curDateTime = QDateTime::currentDateTime();

  m_id =
      m_id.asprintf("%04d%02d%02d%02d%02d%02d%03d", curDateTime.date().year(),
                    curDateTime.date().month(), curDateTime.date().day(),
                    curDateTime.time().hour(), curDateTime.time().minute(),
                    curDateTime.time().second(), curDateTime.time().msec());
}
