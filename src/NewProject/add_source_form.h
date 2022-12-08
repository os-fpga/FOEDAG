#ifndef ADDSOURCEFORM_H
#define ADDSOURCEFORM_H

#include <QWidget>

#include "SettingsGuiInterface.h"
#include "source_grid.h"

namespace Ui {
class addSourceForm;
}

namespace FOEDAG {

class addSourceForm : public QWidget, public SettingsGuiInterface {
  Q_OBJECT

 public:
  explicit addSourceForm(GridType gt, QWidget *parent = nullptr);
  ~addSourceForm();
  void setProjectType(int projectType);
  int projectType() const;
  void clear();

  void SetBasePath(const QString &p);

  QList<filedata> getFileData();
  bool IsCopySource();
  QString TopModule() const;
  QString LibraryForTopModule() const;
  QString LibraryPath() const;
  QString LibraryExt() const;
  QString IncludePath() const;
  QString Macros() const;
  void updateUi(ProjectManager *pm) override;
  void SetTitle(const QString &title);

 private slots:
  void includePathClicked();
  void libraryPathClicked();

 private:
  static QString GetRelatedPath(QWidget *parent, const QString &base);

 private:
  Ui::addSourceForm *ui;

  sourceGrid *m_widgetGrid;
  QString m_basePath;
};
}  // namespace FOEDAG
#endif  // ADDSOURCEFORM_H
