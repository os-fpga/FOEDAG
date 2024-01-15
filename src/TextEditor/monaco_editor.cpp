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
