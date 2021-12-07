#ifndef TEXT_EDITOR_FORM_H
#define TEXT_EDITOR_FORM_H

#include <QTabWidget>
#include <QWidget>

#include "editor.h"
#include "search_dialog.h"

namespace FOEDAG {

class TextEditorForm : public QWidget {
  Q_OBJECT

 public:
  static TextEditorForm *Instance();

  void InitForm();
  int OpenFile(const QString &strFileName);

 public slots:
  void SlotTabCloseRequested(int index);
  void SlotUpdateTabTitle(bool m);
  void SlotShowSearchDialog(const QString &strWord);
  void SlotFind(const QString &strFindWord);
  void SlotFindNext(const QString &strFindWord);
  void SlotReplace(const QString &strFindWord, const QString &strDesWord);
  void SlotReplaceAndFind(const QString &strFindWord,
                          const QString &strDesWord);
  void SlotReplaceAll(const QString &strFindWord, const QString &strDesWord);

 private:
  QTabWidget *m_tab_editor;
  QMap<QString, QPair<int, Editor *>> m_map_file_tabIndex_editor;

  SearchDialog *m_searchDialog;
};
}  // namespace FOEDAG
#endif  // TEXT_EDITOR_FORM_H
