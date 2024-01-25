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
class QObject;
class QWidget;
class QVBoxLayout;
class QWebEngineView;
class QWebChannel;

namespace FOEDAG {

class CPPEndPoint;
class MonacoEditorPage;

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

  void markLineError(int line);
  void markLineWarning(int line);
  void clearMarkers();
  void reload();
  void selectLines(int lineFrom, int lineTo);
  bool fileLoaded() const;

 signals:
  void EditorModificationChanged(bool m);

 public slots:
  void Save();
  void SaveAndClose();
  // handle signals from JS side
  void handleSignalFromJS_SaveFileContent(QVariant fileContent);

 private slots:

  void handleSignalFromJS_FileModified(bool m);

 private:
  bool m_closeAfterSave;
  QString m_strFileName;
  QWebEngineView* m_webEngineView;
  MonacoEditorPage* m_webEngineViewPage;
  QWebChannel* m_webEngineChannel;
  CPPEndPoint* m_CPPEndPointObject;

  QAction* m_actSave;
  QAction* saveCurrentFileAction;

  QFileSystemWatcher* m_fileWatcher{nullptr};
};

}  // namespace FOEDAG

#endif  // EDITOR_H
