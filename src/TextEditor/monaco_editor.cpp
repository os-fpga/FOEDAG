#include "monaco_editor.h"
#include "cpp_endpoint.h"

#include <QCoreApplication>
#include <QDebug>
#include <QWidget>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebChannel>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QFile>
#include <QAction>
#include <QTimer>

#include "Main/Foedag.h"
#include "Main/ToolContext.h"
#include "MainWindow/Session.h"

extern FOEDAG::Session* GlobalSession;

using namespace FOEDAG;

#define ERROR_MARKER 4
#define WARN_MARKER 5

Editor::Editor(QString strFileName, int iFileType, QWidget *parent)
    : QWidget(parent) {
  m_strFileName = strFileName;
  
  QVBoxLayout* monacoTextEditorVBoxLayout = new QVBoxLayout();

  // create the web engine view to hold the HTML content
  m_webEngineView = new QWebEngineView(this);

  // create a separate 'end-point' object to handle comms between C++ and JS
  m_CPPEndPointObject = new CPPEndPoint(this, m_strFileName);

  m_webEngineView->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
  m_webEngineView->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

  // create a webchannel for comms between C++ and JS and register the 'end-point' object to be used
  // on the C++ side to handle the commns
  m_webEngineChannel = new QWebChannel(this);
  m_webEngineChannel->registerObject(QStringLiteral("CPPEndPoint"), m_CPPEndPointObject);

  // set the webchannel on the web engine view's page
  m_webEngineView->page()->setWebChannel(m_webEngineChannel);

  // load the HTML page
  std::filesystem::path monaco_editor_html_path = GlobalSession->Context()->DataPath() / "etc" / "monaco-editor" / "monaco-editor.html";
  m_webEngineView->page()->load(QUrl::fromLocalFile(QString::fromStdString(monaco_editor_html_path.string())));

  // did the HTML page load ok?
  QObject::connect(
    m_webEngineView->page(),
    &QWebEnginePage::loadFinished,
    [this](bool ok) {
      qDebug() << "URL: " << this->m_webEngineView->url().toString();
      qDebug() << "Load Finished: " << ok;

      if (!ok)
      {
        // exit the application here?
      }

      qDebug() << this->m_strFileName;
      emit m_CPPEndPointObject->signalToJS_UpdateFilePath(this->m_strFileName);
    }
  );

  monacoTextEditorVBoxLayout->addWidget(m_webEngineView);
  this->setLayout(monacoTextEditorVBoxLayout);

  // setup Connection to CPPEndPoint to receive events from JS side
  QObject::connect(m_CPPEndPointObject,
                   &CPPEndPoint::signalToCPP_SaveFileContentFromJS,
                   this,
                   &Editor::handleSignalFromJS_SaveFileContent);

  // setup Actions
  saveCurrentFileAction = new QAction(this);
  // https://stackoverflow.com/questions/1346964/use-qaction-without-adding-to-menu-or-toolbar
  this->addAction(saveCurrentFileAction);
  saveCurrentFileAction->setShortcutContext(Qt::ApplicationShortcut);
  saveCurrentFileAction->setShortcut(QKeySequence::Save);
  connect(saveCurrentFileAction,
          &QAction::triggered,
          this,
          &Editor::Save);

  // connect(m_scintilla, SIGNAL(modificationChanged(bool)), this,
  //         SLOT(QscintillaModificationChanged(bool)));
}

QString Editor::getFileName() const { return m_strFileName; }

bool Editor::isModified() const { return false; }

void Editor::SetFileWatcher(QFileSystemWatcher *watcher) {
  m_fileWatcher = watcher;
}

void Editor::markLineError(int line) {
}

void Editor::markLineWarning(int line) {
}

void Editor::selectLines(int lineFrom, int lineTo) {
}

void Editor::clearMarkers() {  }

void Editor::reload() {  }

void Editor::Save() {
  // QFile file(m_strFileName);
  // if (!file.open(QFile::WriteOnly)) {
  //   return;
  // }

  // avoid trigger file watching during save
  m_fileWatcher->removePath(m_strFileName);
  // QTextStream out(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  // out << m_scintilla->text();
  QApplication::restoreOverrideCursor();

  // m_scintilla->setModified(false);
  // QTimer::singleShot is used here to run lambda at the end of the event
  // queue. We must wait all events because file is saved to device later.
  QTimer::singleShot(1, this,
                     [this]() { m_fileWatcher->addPath(m_strFileName); });
}

void Editor::QscintillaModificationChanged(bool m) {
  m_actSave->setEnabled(m);
  emit EditorModificationChanged(m);
}

void Editor::handleSignalFromJS_SaveFileContent(QVariant fileContent) {

//   qDebug() << "saveCurrentFileContentFromJS()";
//   qDebug() << "currentFilePath:" << currentFilePath;
  

//   QFile fileToWrite(currentFilePath);
//   if(!fileToWrite.open(QIODevice::WriteOnly)) {
//     qDebug() << "error in opening file to write!";
//     fileToWrite.close();
//   }
//   else {
//     QTextStream out(&fileToWrite);
// #if (QT_VERSION < QT_VERSION_CHECK(6, 5, 3))
//     qDebug() << "codec:" << out.codec()->name();
//     // out.setCodec("UTF-8");
// #else
//     qDebug() << "codec:" << out.encoding();
//     // out.setEncoding(QStringConverter::Encoding::Utf8);
// #endif
//     out << fileContent.toString();
//     fileToWrite.close();
//     qDebug() << "write to file done.";
//   }
}

void Editor::openFileInCurrentTab(QString filepath) {

  emit m_CPPEndPointObject->signalToJS_UpdateFilePath(filepath);
}