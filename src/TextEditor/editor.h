#ifndef EDITOR_H
#define EDITOR_H

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "Qsci/qsciapis.h"
#include "Qsci/qscilexertcl.h"
#include "Qsci/qscilexerverilog.h"
#include "Qsci/qscilexervhdl.h"

namespace FOEDAG {

enum FileType {
  FILE_TYPE_VERILOG,
  FILE_TYPE_VHDL,
  FILE_TYPE_TCL,
  FILE_TYPE_UNKOWN
};

class Editor : public QWidget {
  Q_OBJECT
 public:
  explicit Editor(QString strFileName, int iFileType,
                  QWidget* parent = nullptr);

 signals:

 private:
  QsciScintilla* m_scintilla;

  void InitScintilla(int iFileType);
  void SetScintillaText(QString strFileName);
};

}  // namespace FOEDAG
#endif  // EDITOR_H
