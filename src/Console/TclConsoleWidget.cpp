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
  setPrompt("# ");
  setTabAllowed(false);
}

bool TclConsoleWidget::isRunning() const { return !m_command_done; }

void TclConsoleWidget::clearText() {
  clear();
  displayPrompt();
}

QString TclConsoleWidget::interpretCommand(const QString &command, int *res) {
  if (!command.isEmpty()) {
    setUndoRedoEnabled(false);
    m_command_done = false;
    QString prepareCommand = command;
    QString histCommand;
    if (handleCommandFromHistory(command, histCommand))
      prepareCommand = histCommand;
    if (m_console) m_console->run(prepareCommand.toUtf8());
    return QConsole::interpretCommand(prepareCommand, res);
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

void TclConsoleWidget::handleSearch() { emit searchEnable(); }

void TclConsoleWidget::put(const QString &str) {
  if (!str.isEmpty()) {
    int res = m_console ? m_console->returnCode() : 0;
    QString strRes = str;
    // According to the return value, display the result either in red or in
    // blue
    setTextColor((res == 0) ? outColor_ : errColor_);

    if (!(strRes.isEmpty() || strRes.endsWith("\n"))) strRes.append("\n");
    textCursor().insertText(strRes);
    moveCursor(QTextCursor::End);
  }
}

void TclConsoleWidget::commandDone() {
  m_command_done = true;
  if (!hasPrompt()) displayPrompt();
}

void TclConsoleWidget::updateScroll() {
  QScrollBar *bar = verticalScrollBar();
  bar->setValue(bar->maximum());
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
    QStringList history{};
    for (QStringList::Iterator it = console->history.begin();
         it != console->history.end(); ++it) {
      history.append(QString("%1\t%2").arg(index).arg(*it));
      index++;
    }
    if (!history.isEmpty()) {
      Tcl_AppendResult(interp, qPrintable(history.join("\n")), (char *)NULL);
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

bool TclConsoleWidget::hasPrompt() const {
  auto lastBlock = document()->lastBlock();
  return !lastBlock.text().isEmpty();
}

bool TclConsoleWidget::handleCommandFromHistory(const QString &command,
                                                QString &commandFromHist) {
  if (command.startsWith("!")) {
    bool ok;
    int cmdNumber = command.midRef(1).toInt(&ok);
    if (ok && cmdNumber >= 1 && cmdNumber <= historyIndex) {
      commandFromHist = history.at(cmdNumber - 1);
      return true;
    }
  }
  return false;
}
