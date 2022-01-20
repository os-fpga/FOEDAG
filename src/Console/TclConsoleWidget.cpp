#include "TclConsoleWidget.h"

#include <QDebug>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMetaMethod>
#include <QScrollBar>
#include <QTextBlock>

#include "StreamBuffer.h"

TclConsoleWidget::TclConsoleWidget(Tcl_Interp *interp,
                                   std::unique_ptr<ConsoleInterface> iConsole,
                                   StreamBuffer *buffer, QWidget *parent)
    : QConsole(parent), m_console(std::move(iConsole)), m_buffer{buffer} {
  connect(m_buffer, &StreamBuffer::ready, this, &TclConsoleWidget::put);
  if (m_console) {
    connect(m_console.get(), &ConsoleInterface::done, this,
            &TclConsoleWidget::commandDone);
    registerCommands(interp);
  }
  commandDone();
  setPrompt("# ");
}

void TclConsoleWidget::clearText() {
  clear();
  displayPrompt();
}

QString TclConsoleWidget::interpretCommand(const QString &command, int *res) {
  if (!command.isEmpty()) {
    if (m_console) m_console->run(command.toUtf8());
    return QConsole::interpretCommand(command, res);
  }
  return QString();
}

QStringList TclConsoleWidget::suggestCommand(const QString &cmd,
                                             QString &prefix) {
  return m_console ? m_console->suggestCommand(cmd, prefix) : QStringList();
}

bool TclConsoleWidget::isCommandComplete(const QString &command) {
  return m_console ? m_console->isCommandComplete(command) : true;
}

void TclConsoleWidget::put(const QString &str) {
  if (!str.isEmpty()) {
    int res = m_console ? m_console->returnCode() : 0;
    QString strRes = str;
    // According to the return value, display the result either in red or in
    // blue
    setTextColor((res == 0) ? outColor_ : errColor_);

    if (!(strRes.isEmpty() || strRes.endsWith("\n"))) strRes.append("\n");
    append(strRes);
    moveCursor(QTextCursor::End);
    // Display the prompt again
    displayPrompt();
  }
}

void TclConsoleWidget::commandDone() {}

void TclConsoleWidget::updateScroll() {
  QScrollBar *bar = verticalScrollBar();
  bar->setValue(bar->maximum());
}

QString TclConsoleWidget::getCommand() const {
  QString cmd = document()->lastBlock().text();
  return cmd.mid(m_beginPosition - 1);
}

void TclConsoleWidget::handleLink(const QPoint &p) { qDebug() << anchorAt(p); }

void TclConsoleWidget::registerCommands(Tcl_Interp *interp) {
  auto hist = [](ClientData clientData, Tcl_Interp *interp, int argc,
                 const char *argv[]) {
    TclConsoleWidget *console = static_cast<TclConsoleWidget *>(clientData);
    if (!console) return TCL_ERROR;
    // Reset result data
    Tcl_ResetResult(interp);

    // Help message in case of wrong parameters
    if (argc != 1) {
      QString usageMsg = QString("Usage: %1\n").arg(argv[0]);
      Tcl_AppendResult(interp, qPrintable(usageMsg), (char *)NULL);
      return TCL_ERROR;
    }

    uint index = 1;
    for (QStringList::Iterator it = console->history.begin();
         it != console->history.end(); ++it) {
      Tcl_AppendResult(interp,
                       qPrintable(QString("%1\t%2\n").arg(index).arg(*it)),
                       (char *)NULL);
      index++;
    }
    return TCL_OK;
  };
  Tcl_CreateCommand(interp, "history", hist, this, nullptr);

  auto set_prompt = [](ClientData clientData, Tcl_Interp *interp, int argc,
                       const char *argv[]) {
    TclConsoleWidget *console = static_cast<TclConsoleWidget *>(clientData);
    if (!console) return TCL_ERROR;
    // Reset result data
    Tcl_ResetResult(interp);

    // Help message in case of wrong parameters
    if (argc != 2) {
      QString usageMsg = QString("Usage: %1 new_prompt\n").arg(argv[0]);
      Tcl_AppendResult(interp, qPrintable(usageMsg), (char *)NULL);
      return TCL_ERROR;
    }
    console->setPrompt(argv[1], false);
    return TCL_OK;
  };
  Tcl_CreateCommand(interp, "set_prompt", set_prompt, this, nullptr);

  auto clear_ = [](ClientData clientData, Tcl_Interp *interp, int argc,
                   const char *argv[]) {
    TclConsoleWidget *console = static_cast<TclConsoleWidget *>(clientData);
    if (!console) return TCL_ERROR;
    Tcl_ResetResult(interp);
    if (argc != 1) {
      QString usageMsg = QString("Usage: %1 clear\n").arg(argv[0]);
      Tcl_AppendResult(interp, qPrintable(usageMsg), (char *)NULL);
      return TCL_ERROR;
    }
    // need to put it the event queue otherwise it will crash
    int methodIndex = console->metaObject()->indexOfMethod("clearText()");
    QMetaMethod method = console->metaObject()->method(methodIndex);
    method.invoke(console, Qt::QueuedConnection);
    return TCL_OK;
  };

  Tcl_CreateCommand(interp, "clear", clear_, this, nullptr);
}
