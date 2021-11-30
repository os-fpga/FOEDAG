#ifndef TEXT_EDITOR_FORM_H
#define TEXT_EDITOR_FORM_H

#include <QTabWidget>
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

class TextEditorForm : public QWidget {
  Q_OBJECT

 public:
  static TextEditorForm *Instance();

  void InitForm();
  int OpenFile(const QString &strFileName);

 private:
  QTabWidget *m_tab_editor;
  QMap<QString, QPair<int, QsciScintilla *>> m_map_file_tabIndex_editor;

  QsciScintilla *CreateScintilla(int iFileType);
};
}  // namespace FOEDAG
#endif  // TEXT_EDITOR_FORM_H
