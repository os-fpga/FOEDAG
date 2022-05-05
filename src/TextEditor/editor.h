#ifndef EDITOR_H
#define EDITOR_H

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTextStream>
#include <QToolBar>
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

  QString getFileName() const;
  bool isModified() const;

  void FindFirst(const QString& strWord);
  void FindNext(const QString& strWord);
  void Replace(const QString& strFind, const QString& strDesWord);
  void ReplaceAndFind(const QString& strFind, const QString& strDesWord);
  void ReplaceAll(const QString& strFind, const QString& strDesWord);
  void markLine(int line);
  void clearMarkers();

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
};

}  // namespace FOEDAG
#endif  // EDITOR_H
