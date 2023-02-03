#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H
#include <QObject>

#include "project_option.h"
namespace FOEDAG {

class ProjectConfiguration : public ProjectOption {
  Q_OBJECT
 public:
  explicit ProjectConfiguration(QObject *parent = nullptr);

  QString id() const;
  void setId(const QString &id);

  int projectType() const;
  void setProjectType(int projectType);

  QString activeSimSet() const;
  void setActiveSimSet(const QString &activeSimSet);

 private:
  int m_projectType;
  QString m_id;
  QString m_activeSimSet;

  void initProjectID();
};
}  // namespace FOEDAG
#endif  // PROJECTCONFIG_H
