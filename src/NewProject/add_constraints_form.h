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

 private slots:
  void on_select_random_clicked(bool random_checked);

 private slots:
  void on_select_defineOrder_clicked(bool define_order_checked);

 private:
  Ui::addConstraintsForm *ui;

  sourceGrid *m_widgetGrid;
};
}  // namespace FOEDAG
#endif  // ADDCONSTRAINTSFORM_H
