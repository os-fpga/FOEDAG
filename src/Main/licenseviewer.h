#pragma once

#include <QDialog>
#include <filesystem>

namespace FOEDAG {
// Scrollable read-only viewer for licenses information
class LicenseViewer : public QDialog {
 public:
  LicenseViewer(const std::filesystem::path &licensesDir,
                const QString &appName, QWidget *parent = nullptr);

 private:
  QString getFileContent(const std::filesystem::path &filePath) const;
};
}  // namespace FOEDAG
