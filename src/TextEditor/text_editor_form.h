#ifndef TEXT_EDITOR_FORM_H
#define TEXT_EDITOR_FORM_H

#include <QFileSystemWatcher>
#include <QResizeEvent>
#include <QTabWidget>

#include "editor.h"
#include "search_dialog.h"

namespace FOEDAG {

class TabWidget : public QTabWidget {
  Q_OBJECT
 public:
  explicit TabWidget(QWidget *parent = nullptr);

 signals:
  void resized(const QSize &newSize);

 protected:
  void resizeEvent(QResizeEvent *event) override;
};

class TextEditorForm : public QWidget {
  Q_OBJECT

 public:
  static TextEditorForm *Instance();

  void InitForm();
  int OpenFile(const QString &strFileName);
  int OpenFileWithLine(const QString &strFileName, int line, bool error = true);
  int OpenFileWithSelection(const QString &strFileName, int lineFrom,
                            int lineTo);
  TabWidget *GetTabWidget() { return m_tab_editor; }
  bool TabCloseRequested(int index);

 signals:
  void CurrentFileChanged(QString);
  void FileChanged(const QString &);

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
  TabWidget *m_tab_editor;
  QMap<QString, QPair<int, Editor *>> m_map_file_tabIndex_editor;

  SearchDialog *m_searchDialog;
  QFileSystemWatcher m_fileWatcher;
};
}  // namespace FOEDAG
#endif  // TEXT_EDITOR_FORM_H
