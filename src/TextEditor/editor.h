#ifndef EDITOR_H
#define EDITOR_H

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

class QsciScintilla;

namespace FOEDAG {

enum FileType {
  FILE_TYPE_VERILOG,
  FILE_TYPE_VHDL,
  FILE_TYPE_TCL,
  FILE_TYPE_CPP,
  FILE_TYPE_UNKOWN
};

class Editor : public QWidget {
  Q_OBJECT
 public:
  explicit Editor(QString strFileName, int iFileType,
                  QWidget* parent = nullptr);

  QString getFileName() const;
  bool isModified() const;
  void SetFileWatcher(QFileSystemWatcher* watcher);

  void FindFirst(const QString& strWord);
  void FindNext(const QString& strWord);
  void Replace(const QString& strFind, const QString& strDesWord);
  void ReplaceAndFind(const QString& strFind, const QString& strDesWord);
  void ReplaceAll(const QString& strFind, const QString& strDesWord);
  void markLineError(int line);
  void markLineWarning(int line);
  void clearMarkers();
  void reload();
  void selectLines(int lineFrom, int lineTo);

 signals:
  void EditorModificationChanged(bool m);
  void ShowSearchDialog(QString);

 public slots:
  void Save();

 private slots:
  void Search();
  void Undo();
  void Redo();
  void Cut();
  void Copy();
  void Paste();
  void Delete();
  void SelectAll();

  void QScintillaTextChanged();
  void QscintillaSelectionChanged();
  void QscintillaModificationChanged(bool m);
  void QscintillaLinesChanged();

 private:
  QString m_strFileName;
  QsciScintilla* m_scintilla;

  QToolBar* m_toolBar;
  QAction* m_actSearch;
  QAction* m_actSave;

  QAction* m_actUndo;
  QAction* m_actRedo;

  QAction* m_actCut;
  QAction* m_actCopy;
  QAction* m_actPaste;
  QAction* m_actDelete;

  QAction* m_actSelect;

  void InitToolBar();
  void InitScintilla(int iFileType);
  void SetScintillaText(QString strFileName);

  void UpdateToolBarStates();
  QFileSystemWatcher* m_fileWatcher{nullptr};
  static constexpr int MIN_MARGIN_WIDTH{4};
  static constexpr int MARGIN_INDEX{0};
  int m_marginWidth{MIN_MARGIN_WIDTH};
};

}  // namespace FOEDAG
#endif  // EDITOR_H
