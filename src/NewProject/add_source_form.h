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
  explicit addSourceForm(QWidget *parent = nullptr);
  ~addSourceForm();

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

 private:
  Ui::addSourceForm *ui;

  sourceGrid *m_widgetGrid;
};
}  // namespace FOEDAG
#endif  // ADDSOURCEFORM_H
