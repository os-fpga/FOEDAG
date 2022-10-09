#ifndef ADDCONSTRAINTSFORM_H
#define ADDCONSTRAINTSFORM_H

#include <QWidget>

#include "SettingsGuiInterface.h"
#include "source_grid.h"

namespace Ui {
class addConstraintsForm;
}

namespace FOEDAG {

class addConstraintsForm : public QWidget, public SettingsGuiInterface {
  Q_OBJECT

 public:
  explicit addConstraintsForm(QWidget *parent = nullptr);
  ~addConstraintsForm();

  QList<filedata> getFileData();
  bool IsCopySource();
  bool IsRandom() const;
  void updateUi(ProjectManager *pm) override;
  void SetTitle(const QString &title);

 private:
  Ui::addConstraintsForm *ui;

  sourceGrid *m_widgetGrid;
};
}  // namespace FOEDAG
#endif  // ADDCONSTRAINTSFORM_H
