
#include "monaco_editor_page.h"

#include <QWebEnginePage>

using namespace FOEDAG;

MonacoEditorPage::MonacoEditorPage(QObject *parent) : QWebEnginePage(parent) {}

void MonacoEditorPage::javaScriptConsoleMessage(
    JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber,
    const QString &sourceID) {
  // do nothing, all messages from JS console are suppressed!
  // qDebug() << message;
}
