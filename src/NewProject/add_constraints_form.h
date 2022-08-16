#ifndef ADDCONSTRAINTSFORM_H
#define ADDCONSTRAINTSFORM_H

#include <QWidget>

#include "source_grid.h"

namespace Ui {
class addConstraintsForm;
}

namespace FOEDAG {

class addConstraintsForm : public QWidget {
  Q_OBJECT

 public:
  explicit addConstraintsForm(QWidget *parent = nullptr);
  ~addConstraintsForm();

  QList<filedata> getFileData();
  bool IsCopySource();

 private:
  Ui::addConstraintsForm *ui;

  sourceGrid *m_widgetGrid;
};
}  // namespace FOEDAG
#endif  // ADDCONSTRAINTSFORM_H
