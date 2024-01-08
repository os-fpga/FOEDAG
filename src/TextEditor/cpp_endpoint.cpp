#include "cpp_endpoint.h"

#include <QApplication>
#include <QDebug>
#include <QVariant>
#include <QTime>


// https://stackoverflow.com/questions/240353/convert-a-preprocessor-token-to-a-string
// https://gcc.gnu.org/onlinedocs/gcc-13.2.0/cpp/Stringizing.html
// if we have '#define foo abcd', then:
// step 1: TOSTRING(foo)  fully macro-expanded -> TOSTRING(abcd) -> STRINGIFY(abcd)
#define TOSTRING(x) STRINGIFY(x)
// step 2: STRINGIFY(abcd) -> replaced by "abcd" and not macro expanded because it is stringized with '#'
#define STRINGIFY(x) #x


CPPEndPoint::CPPEndPoint(QObject* parent, QString filePath) {
  
  // set the Qt Version property:
  m_qtVersion.append(((QT_VERSION) >> 16) & 0xff);
  m_qtVersion.append(((QT_VERSION) >> 8) & 0xff);
  m_qtVersion.append((QT_VERSION) & 0xff);

  m_filePath = filePath;

}


Q_INVOKABLE void CPPEndPoint::log(QVariant s)
{
  QString str = s.toString();
  qDebug() << "log from JS: " << str;
}


Q_INVOKABLE QVariant CPPEndPoint::getAppVersion()
{
  qDebug() << "getAppVersion()";
  
  QString appVersion = TOSTRING(BUILD_VERSION);

  return appVersion;
}


Q_INVOKABLE void CPPEndPoint::saveFileContent(QVariant fileContent)
{
  qDebug() << "saveFileContent() from JS, send to CPP";
  
  emit signalToCPP_SaveFileContentFromJS(fileContent);
}


Q_INVOKABLE void CPPEndPoint::hoveredOnElement(QVariant elementName) {
  qDebug() << "hovered on" << elementName.toString() << QTime::currentTime();
}


Q_INVOKABLE int CPPEndPoint::getIntValue()
{
  qDebug() << "getIntValue()";
  return m_intValue;
}


Q_INVOKABLE QVariant CPPEndPoint::getQtVersion()
{
  QVariant variant = QVariant::fromValue(m_qtVersion);
  qDebug() << "getQtVersion()";
  return variant;
}


Q_INVOKABLE QVariant CPPEndPoint::getFilePath()
{
  QVariant variant = QVariant::fromValue(m_filePath);
  qDebug() << "getFilePath()";
  return variant;
}
