#ifndef NEWFILE_H
#define NEWFILE_H

#include <QFileDialog>
#include <QObject>
#include <QWidget>

#include "new_file_dialog.h"

#define FILTER_VERILOG "Verilog HDL Files(*.v)"
#define FILTER_VHDL "VHDL Files(*.vhd)"
#define FILTER_TCL "Tcl Script Files(*.tcl)"
#define FILTER_CONSTR "Synopsys Design Constraints Files(*.sdc)"
#define FILTER_ALL "All Files(*.*)"

namespace FOEDAG {

class NewFile : public QWidget {
  Q_OBJECT
 public:
  explicit NewFile(QWidget* parent = nullptr);

  void StartNewFile();
  void StopNewFile();

  void TclNewFile(const QString& strFileName);
 signals:
  void OpenFile(QString);

 private slots:
  void SlotFileSelected(const QString& file);
  void SlotFilterSelected(const QString& filter);
  void SlotFinished(int result);

 private:
  NewFileDialog* m_fileDialog;
  QString m_file;
  QString m_filter;

  int CreateFile(QString strFileName);
};
}  // namespace FOEDAG
#endif  // NEWFILE_H
