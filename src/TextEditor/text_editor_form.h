#ifndef TEXT_EDITOR_FORM_H
#define TEXT_EDITOR_FORM_H

#include <QTabWidget>
#include <QWidget>

#include "editor.h"

namespace FOEDAG {

class TextEditorForm : public QWidget {
  Q_OBJECT

 public:
  static TextEditorForm *Instance();

  void InitForm();
  int OpenFile(const QString &strFileName);

 private:
  QTabWidget *m_tab_editor;
  QMap<QString, QPair<int, Editor *>> m_map_file_tabIndex_editor;
};
}  // namespace FOEDAG
#endif  // TEXT_EDITOR_FORM_H
