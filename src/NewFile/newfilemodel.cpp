#include "newfilemodel.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>

using namespace FOEDAG;

NewFileModel::NewFileModel(QObject *parent) : QObject{parent} {}

bool NewFileModel::createNewFile(const QString &fileName) {
  QFile file(fileName);
  if (file.exists()) return true;
  if (!file.open(QFile::WriteOnly | QFile::Text)) return false;

  file.close();
  return true;
}

bool NewFileModel::createNewFileWithExtensionCheck(const QString &fileName,
                                                   const QString &extension) {
  QFileInfo fileInfo(fileName);
  QString suffix = fileInfo.suffix();
  QString newFileName;

  if (suffix.compare(extension, Qt::CaseInsensitive)) {
    newFileName = fileName + "." + extension;
  } else {
    newFileName = fileName;
  }

  bool isNewFileCreated = createNewFile(newFileName);
  if (isNewFileCreated) {
    emit openFile(newFileName);  // not connected
  }
  return isNewFileCreated;
}

QStringList NewFileModel::fileDialogFilters() { return filters; }
