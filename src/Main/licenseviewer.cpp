#include "licenseviewer.h"

#include <QDir>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QTextStream>

namespace FOEDAG {
LicenseViewer::LicenseViewer(const std::filesystem::path &licensesDir,
                             const QString &appName, QWidget *parent)
    : QDialog(parent) {
  auto dialogLayout = new QGridLayout(this);
  auto textEditor = new QPlainTextEdit(this);
  textEditor->setReadOnly(true);

  auto dirPathStr = QString::fromStdString(licensesDir.string());
  auto licenseFiles = QDir(dirPathStr).entryList({"*.txt"}, QDir::Files);

  auto textDoc = textEditor->document();
  for (auto &file : licenseFiles) {
    if (!textDoc->isEmpty()) textEditor->appendPlainText("\n\n\n");
    textEditor->appendPlainText(
        getFileContent(licensesDir / file.toStdString()));
  }

  textEditor->moveCursor(QTextCursor::Start);

  dialogLayout->addWidget(textEditor);
  setLayout(dialogLayout);

  setWindowTitle(QString("%1 Licenses").arg(appName));
}

QString LicenseViewer::getFileContent(
    const std::filesystem::path &filePath) const {
  auto fileContent = QString{};
  auto licenseFile = QFile(QString::fromStdString(filePath.string()));

  if (!licenseFile.open(QIODevice::ReadOnly)) return fileContent;

  auto in = QTextStream(&licenseFile);
  fileContent = in.readAll();
  licenseFile.close();

  return fileContent.trimmed();
}

}  // namespace FOEDAG
