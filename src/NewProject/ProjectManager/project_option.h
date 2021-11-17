#ifndef PROJECTOPTION_H
#define PROJECTOPTION_H

#include <QMap>
#include <QObject>

class ProjectOption : public QObject {
  Q_OBJECT
 public:
  explicit ProjectOption(QObject *parent = nullptr);

  ProjectOption &operator=(const ProjectOption &other);

  void setOption(const QString &strKey, const QString &strValue);
  QString getOption(QString strKey);

  QMap<QString, QString> getMapOption() const;

 private:
  QMap<QString, QString> m_mapOption;
};

#endif  // PROJECTOPTION_H
