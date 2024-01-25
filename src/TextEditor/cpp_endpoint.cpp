#include "cpp_endpoint.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QTime>
#include <QUrl>
#include <QVariant>

// https://stackoverflow.com/questions/240353/convert-a-preprocessor-token-to-a-string
// https://gcc.gnu.org/onlinedocs/gcc-13.2.0/cpp/Stringizing.html
// if we have '#define foo abcd', then:
// step 1: TOSTRING(foo)  fully macro-expanded -> TOSTRING(abcd) ->
// STRINGIFY(abcd)
#define TOSTRING(x) STRINGIFY(x)
// step 2: STRINGIFY(abcd) -> replaced by "abcd" and not macro expanded because
// it is stringized with '#'
#define STRINGIFY(x) #x

using namespace FOEDAG;

CPPEndPoint::CPPEndPoint(QObject* parent, QString filePath) {
  // set the Qt Version property:
  m_qtVersion.append(((QT_VERSION) >> 16) & 0xff);
  m_qtVersion.append(((QT_VERSION) >> 8) & 0xff);
  m_qtVersion.append((QT_VERSION)&0xff);

  // store the filepath passed in:
  m_filePath = filePath;
  m_fileIsModified = false;
  m_fileLoaded = false;
}

Q_INVOKABLE void CPPEndPoint::log(QVariant s) {
  QString str = s.toString();
  qDebug() << "log from JS: " << str;
}

Q_INVOKABLE void CPPEndPoint::fileLoaded() {
  m_fileLoaded = true;
  emit signalToCPP_FileLoaded();
}

void CPPEndPoint::fileFailedToLoad(QVariant file) {
  m_fileLoaded = false;
  qWarning() << "Failed to load file: " << file;
  emit signalToCPP_FileLoaded();
}

Q_INVOKABLE void CPPEndPoint::openLink(QVariant linkURI) {
  // use Qt to open the link:
  QDesktopServices::openUrl(QUrl(linkURI.toString()));
}

Q_INVOKABLE void CPPEndPoint::saveFileContent(QVariant fileContent) {
  // qDebug() << "saveFileContent() from JS, send to CPP";
  emit signalToCPP_SaveFileContentFromJS(fileContent);
}

Q_INVOKABLE void CPPEndPoint::fileContentModified(
    QVariant fileContentModified) {
  // qDebug() << "fileContentModified() from JS, send to CPP" <<
  m_fileIsModified = fileContentModified.toBool();
  emit signalToCPP_FileModified(m_fileIsModified);
}

Q_INVOKABLE QVariant CPPEndPoint::getQtVersion() {
  QVariant variant = m_qtVersion;
  return variant;
}

Q_INVOKABLE void CPPEndPoint::qtVersion(QVariant v) {
  m_qtVersion = v.toList();
}

Q_INVOKABLE QVariant CPPEndPoint::getFilePath() {
  QVariant variant = m_filePath;
  return variant;
}

Q_INVOKABLE void CPPEndPoint::filePath(QVariant v) {
  m_filePath = v.toString();
}
