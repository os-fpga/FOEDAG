#include "monaco_editor.h"

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEngineScript>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QWidget>

#include "Main/Foedag.h"
#include "Main/ToolContext.h"
#include "MainWindow/Session.h"
#include "cpp_endpoint.h"

extern FOEDAG::Session* GlobalSession;

using namespace FOEDAG;

#define ERROR_MARKER 4
#define WARN_MARKER 5

// Known Limitations:
//
// [1] Delay seen when opening editor instance first time
// TODO: Optimize the delay in waiting for the JS side of Monaco Editor to be
// initialized before returning from this ctor (seen on first editor instance
// open)
//
// [2] Delay seen in monaco editor resizing
// TODO: Optimize the delay seen in Automatic Resize of Monaco Editor w.r.t
// resize of the FOEDAG window which seems to be due to the GPU rendering of the
// JS code
//
// [3] Mouse Cursor not changing from 'Arrow' to 'Text' cursor sometimes
// TODO: On clicking outside the editor, cursor changes to 'Arrow' and
// on refocus on the editor, cursor changes to 'Text', however on doing this
// multiple times, sometimes, the 'Arrow' does not change to 'Text' even though
// editor is focused correctly (we can even select the text with the 'Arrow'
// cursor)

Editor::Editor(QString strFileName, int iFileType, QWidget* parent)
    : QWidget(parent) {
  m_strFileName = strFileName;
  m_closeAfterSave = false;

  QVBoxLayout* monacoTextEditorVBoxLayout = new QVBoxLayout();

  // create the web engine view to hold the HTML content
  m_webEngineView = new QWebEngineView(this);

  // create a separate 'end-point' object to handle comms between C++ and JS
  // init it with the filepath to be opened
  m_CPPEndPointObject = new CPPEndPoint(this, m_strFileName);

  m_webEngineView->page()->settings()->setAttribute(
      QWebEngineSettings::LocalContentCanAccessFileUrls, true);
  // ref:
  // https://forum.qt.io/topic/137108/does-qt-support-the-clipboard-api-how-do-i-use-it/2
  // ref:
  // https://github.com/microsoft/monaco-editor/issues/2264
  m_webEngineView->page()->settings()->setAttribute(
      QWebEngineSettings::JavascriptCanAccessClipboard, true);
  m_webEngineView->page()->settings()->setAttribute(
      QWebEngineSettings::JavascriptCanPaste, true);
  m_webEngineView->page()->settings()->setAttribute(
      QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  m_webEngineView->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  // create a webchannel for comms between C++ and JS and register the
  // 'end-point' object to be used on the C++ side to handle the commns
  m_webEngineChannel = new QWebChannel(this);
  m_webEngineChannel->registerObject(QStringLiteral("CPPEndPoint"),
                                     m_CPPEndPointObject);

  // set the webchannel on the web engine view's page
  m_webEngineView->page()->setWebChannel(m_webEngineChannel);

  // load the HTML page
  std::filesystem::path monaco_editor_html_path =
      GlobalSession->Context()->DataPath() / "etc" / "monaco-editor" /
      "monaco-editor.html";
  m_webEngineView->page()->load(QUrl::fromLocalFile(
      QString::fromStdString(monaco_editor_html_path.string())));

  // did the HTML page load ok?
  QObject::connect(m_webEngineView->page(), &QWebEnginePage::loadFinished,
                   [this](bool ok) {
                     if (!ok) {
                       // TODO: exit the application here?
                     }
                   });

  monacoTextEditorVBoxLayout->addWidget(m_webEngineView);
  this->setLayout(monacoTextEditorVBoxLayout);

  // setup Connection to CPPEndPoint to receive events from JS side
  QObject::connect(m_CPPEndPointObject,
                   &CPPEndPoint::signalToCPP_SaveFileContentFromJS, this,
                   &Editor::handleSignalFromJS_SaveFileContent);

  // setup Actions
  saveCurrentFileAction = new QAction(this);
  // https://stackoverflow.com/questions/1346964/use-qaction-without-adding-to-menu-or-toolbar
  this->addAction(saveCurrentFileAction);
  saveCurrentFileAction->setShortcutContext(Qt::ApplicationShortcut);
  saveCurrentFileAction->setShortcut(QKeySequence::Save);
  QObject::connect(saveCurrentFileAction, &QAction::triggered, this,
                   &Editor::Save);

  QObject::connect(m_CPPEndPointObject, &CPPEndPoint::signalToCPP_FileModified,
                   this, &Editor::handleSignalFromJS_FileModified);

  // wait until monaco reports that the file is actually loaded and ready,
  // because on Qt side, code assumes that once the editor instance ctor
  // returns, the file is ready use an eventloop to wait for this, to prevent
  // locking threads.
  QEventLoop pause;

  // the event loop quits once the monaco editor reports that the file has been
  // loaded
  connect(m_CPPEndPointObject, &CPPEndPoint::signalToCPP_FileLoaded, &pause,
          &QEventLoop::quit);

  // below will never happen, as we check for file existence before creating
  // editor instance. safety timer to handle file load failed scenarios QTimer
  // timer; timer.setInterval(2000); // timeout for reading the file!
  // timer.setSingleShot(true);
  // pause.connect(&timer, &QTimer::timeout, &pause, &QEventLoop::quit);
  // timer.start();

  pause.exec();

  // timer.stop();
}

QString Editor::getFileName() const { return m_strFileName; }

bool Editor::isModified() const {
  return m_CPPEndPointObject->m_fileIsModified;
}

void Editor::SetFileWatcher(QFileSystemWatcher* watcher) {
  m_fileWatcher = watcher;
}

void Editor::markLineError(int line) {
  emit m_CPPEndPointObject->signalToJS_SetHighlightError(line, line);
}

void Editor::markLineWarning(int line) {
  emit m_CPPEndPointObject->signalToJS_SetHighlightWarning(line, line);
}

void Editor::selectLines(int lineFrom, int lineTo) {
  emit m_CPPEndPointObject->signalToJS_SetHighlightSelection(lineFrom, lineTo);
}

void Editor::clearMarkers() {
  emit m_CPPEndPointObject->signalToJS_ClearHighlightError();
  emit m_CPPEndPointObject->signalToJS_ClearHighlightWarning();
  emit m_CPPEndPointObject->signalToJS_ClearHighlightSelection();
}

void Editor::reload() {
  emit m_CPPEndPointObject->signalToJS_FilePathChanged(m_strFileName);
  emit EditorModificationChanged(false);
  m_CPPEndPointObject->m_fileIsModified = false;
}

void Editor::Save() {
  // signal to JS side to start the save file process.
  // JS will invoke Qt side resulting in a signal which we handle in
  // handleSignalFromJS_SaveFileContent

  emit m_CPPEndPointObject->signalToJS_SaveFile();
}

void Editor::SaveAndClose() {
  // mark ourself to close after save is done:
  m_closeAfterSave = true;

  // signal to JS side to start the save file process.
  // JS will invoke Qt side resulting in a signal which we handle in
  // handleSignalFromJS_SaveFileContent

  emit m_CPPEndPointObject->signalToJS_SaveFile();
}

void Editor::handleSignalFromJS_FileModified(bool m) {
  // m_actSave->setEnabled(m);
  emit EditorModificationChanged(m);
}

void Editor::handleSignalFromJS_SaveFileContent(QVariant fileContent) {
  //  qDebug() << "handleSignalFromJS_SaveFileContent()";

  QFile fileToWrite(m_strFileName);
  if (!fileToWrite.open(QIODevice::WriteOnly)) {
    // qDebug() << "error in opening file to write!";
    fileToWrite.close();
    return;
  }

  // avoid trigger file watching during save
  m_fileWatcher->removePath(m_strFileName);

  QTextStream out(&fileToWrite);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  out << fileContent.toString();
  fileToWrite.close();

  QApplication::restoreOverrideCursor();

  emit EditorModificationChanged(false);
  m_CPPEndPointObject->m_fileIsModified = false;

  if (m_closeAfterSave) {
    deleteLater();
  } else {
    // QTimer::singleShot is used here to run lambda at the end of the event
    // queue. We must wait all events because file is saved to device later.
    QTimer::singleShot(1, this,
                       [this]() { m_fileWatcher->addPath(m_strFileName); });
  }
}
