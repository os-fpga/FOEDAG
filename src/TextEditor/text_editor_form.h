#ifndef TEXT_EDITOR_FORM_H
#define TEXT_EDITOR_FORM_H

#include <QFileSystemWatcher>
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
  int OpenFileWithLine(const QString &strFileName, int line);
  QTabWidget *GetTabWidget() { return m_tab_editor; }

 signals:
  void CurrentFileChanged(QString);

 private slots:
  void SlotTabCloseRequested(int index);
  void SlotCurrentChanged(int index);

  void SlotUpdateTabTitle(bool m);

  void SlotShowSearchDialog(const QString &strWord);

  void SlotFind(const QString &strFindWord);
  void SlotFindNext(const QString &strFindWord);
  void SlotReplace(const QString &strFindWord, const QString &strDesWord);
  void SlotReplaceAndFind(const QString &strFindWord,
                          const QString &strDesWord);
  void SlotReplaceAll(const QString &strFindWord, const QString &strDesWord);
  void fileModifiedOnDisk(const QString &path);

 private:
  QTabWidget *m_tab_editor;
  QMap<QString, QPair<int, Editor *>> m_map_file_tabIndex_editor;

  SearchDialog *m_searchDialog;
  QFileSystemWatcher m_fileWatcher;
};
}  // namespace FOEDAG
#endif  // TEXT_EDITOR_FORM_H
