#ifndef MONACO_EDITOR_PAGE_H
#define MONACO_EDITOR_PAGE_H

#include <QWebEnginePage>

class MonacoEditorPage : public QWebEnginePage {
  Q_OBJECT
 public:
  MonacoEditorPage(QObject *parent = 0);
  virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                        const QString &message, int lineNumber,
                                        const QString &sourceID);
};

#endif  // MONACO_EDITOR_PAGE_H