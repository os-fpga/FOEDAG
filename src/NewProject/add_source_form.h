#ifndef ADDSOURCEFORM_H
#define ADDSOURCEFORM_H

#include <QWidget>

#include "source_grid.h"

namespace Ui {
class addSourceForm;
}

namespace FOEDAG {

class addSourceForm : public QWidget {
  Q_OBJECT

 public:
  explicit addSourceForm(QWidget *parent = nullptr);
  ~addSourceForm();

  QList<filedata> getFileData();
  bool IsCopySource();
  QString TopModule() const;
  QString LibraryForTopModule() const;

 private:
  Ui::addSourceForm *ui;

  sourceGrid *m_widgetGrid;
};
}  // namespace FOEDAG
#endif  // ADDSOURCEFORM_H
