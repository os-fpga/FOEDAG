#ifndef ADDCONSTRAINTSFORM_H
#define ADDCONSTRAINTSFORM_H

#include <QWidget>

#include "source_grid.h"

extern bool flag;
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
  void pinAssign_flag_listen();
  void pinAssign_flag_listen_2();

 private:
  Ui::addConstraintsForm *ui;

  sourceGrid *m_widgetGrid;
};

}  // namespace FOEDAG
#endif  // ADDCONSTRAINTSFORM_H
