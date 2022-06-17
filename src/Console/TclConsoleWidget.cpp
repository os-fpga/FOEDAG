#include "TclConsoleWidget.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMetaMethod>
#include <QProcess>
#include <QScrollBar>
#include <QStack>
#include <QTextBlock>

#include "Compiler/Log.h"
#include "ConsoleDefines.h"
#include "FileInfo.h"
#include "StreamBuffer.h"

extern FOEDAG::Session *GlobalSession;

namespace FOEDAG {

Q_GLOBAL_STATIC_WITH_ARGS(QString, linkSep, {"::"})

TclConsoleWidget::TclConsoleWidget(TclInterp *interp,
                                   std::unique_ptr<ConsoleInterface> iConsole,
                                   StreamBuffer *buffer, QWidget *parent)
    : QConsole(parent),
      m_console(std::move(iConsole)),
      m_buffer{buffer},
      m_errorBuffer(new StreamBuffer) {
  connect(m_buffer, &StreamBuffer::ready, this, &TclConsoleWidget::put);
  connect(m_errorBuffer, &StreamBuffer::ready, this,
          &TclConsoleWidget::putError);
  m_formatter.setTextEdit(this);
  if (m_console) {
    connect(m_console.get(), &ConsoleInterface::done, this,
            &TclConsoleWidget::commandDone);
    connect(m_console.get(), &ConsoleInterface::inProgressState, this,
            [this]() { setState(State::IN_PROGRESS); });
    m_console->setErrorStream(&m_errorBuffer->getStream());
    registerCommands(interp);
  }
  setPrompt("# ");
  setTabAllowed(false);
  setMouseTracking(true);
  setObjectName(consoleObjectName());
  setLineWrapMode(QTextEdit::NoWrap);
}

bool TclConsoleWidget::isRunning() const {
  return state() == State::IN_PROGRESS;
}

QString TclConsoleWidget::getPrompt() const { return prompt; }

StreamBuffer *TclConsoleWidget::getBuffer() { return m_buffer; }

StreamBuffer *TclConsoleWidget::getErrorBuffer() { return m_errorBuffer; }

const char *TclConsoleWidget::consoleObjectName() { return "TclConsole"; }

void TclConsoleWidget::clearText() {
  clear();
  displayPrompt();
}

QString TclConsoleWidget::interpretCommand(const QString &command, int *res) {
  if (!command.isEmpty()) {
    setUndoRedoEnabled(false);
    setState(State::IN_PROGRESS);
    QString prepareCommand = command;
    QString histCommand;
    if (handleCommandFromHistory(command, histCommand))
      prepareCommand = histCommand;
    QConsole::interpretCommand(prepareCommand, res);
    if (m_console) {
      LOG_CMD(command);
      m_console->run(prepareCommand.toUtf8());
    }
    setMultiLine(false);
    return QString();
  }
  return QString();
}

QStringList TclConsoleWidget::suggestCommand(const QString &cmd,
                                             QString &prefix) {
  return m_console ? m_console->suggestCommand(cmd, prefix) : QStringList();
}

bool TclConsoleWidget::isCommandComplete(const QString &command) {
  if (isMultiLine()) {
    const bool complete = hasCloseBracket(command);
    if (complete) setMultiLine(false);
    return complete;
  } else {
    if (hasOpenBracket(command)) {
      setMultiLine(true);
      return false;
    }
  }
  return m_console ? m_console->isCommandComplete(command) : true;
}

void TclConsoleWidget::handleSearch() { emit searchEnable(); }

void TclConsoleWidget::handleTerminateCommand() {
  if (state() == State::IN_PROGRESS)
    if (m_console) m_console->abort();
  QConsole::handleTerminateCommand();
}

void TclConsoleWidget::mouseReleaseEvent(QMouseEvent *e) {
  if (m_linkActivated && m_mouseButtonPressed == Qt::LeftButton)
    handleLink(e->pos());

  // Mouse was released, activate links again
  m_linkActivated = true;
  m_mouseButtonPressed = Qt::NoButton;

  QConsole::mouseReleaseEvent(e);
}

void TclConsoleWidget::mousePressEvent(QMouseEvent *e) {
  m_mouseButtonPressed = e->button();
  QConsole::mousePressEvent(e);
}

void TclConsoleWidget::mouseMoveEvent(QMouseEvent *e) {
  // Cursor was dragged to make a selection, deactivate links
  if (m_mouseButtonPressed != Qt::NoButton && textCursor().hasSelection())
    m_linkActivated = false;

  if (!m_linkActivated || anchorAt(e->pos()).isEmpty())
    viewport()->setCursor(Qt::IBeamCursor);
  else
    viewport()->setCursor(Qt::PointingHandCursor);
  QConsole::mouseMoveEvent(e);
}

void TclConsoleWidget::put(const QString &str) { putMessage(str, Output); }

void TclConsoleWidget::putError(const QString &str) { putMessage(str, Error); }

void TclConsoleWidget::commandDone() {
  if (!hasPrompt()) displayPrompt();
  setState(State::IDLE);
}

void FOEDAG::TclConsoleWidget::putMessage(const QString &message,
                                          OutputFormat format) {
  if (!message.isEmpty()) {
    moveCursor(QTextCursor::End);
    LOG_OUTPUT(message);
    m_formatter.appendMessage(message, format);
  }
}

void TclConsoleWidget::handleLink(const QPoint &p) {
  const QString anchor{anchorAt(p)};
  if (!anchor.isEmpty()) {
    auto fileInfo = anchor.split(*linkSep());
    ErrorInfo info{fileInfo.at(0),
                   (fileInfo.count() >= 2) ? fileInfo.at(1) : QString{"-1"}};
    emit linkActivated(info);
  }
}

void TclConsoleWidget::registerCommands(TclInterp *interp) {
  auto hist = [](ClientData clientData, Tcl_Interp *interp, int argc,
                 const char *argv[]) {
    TclConsoleWidget *console = static_cast<TclConsoleWidget *>(clientData);
    // Reset result data
    Tcl_ResetResult(interp);

    // Help message in case of wrong parameters
    if (argc > 1) {
      QString secondArg = argv[1];
      if (secondArg == "clear") {
        console->history.clear();
        console->historyIndex = 0;
        return TCL_OK;
      } else {
        QString usageMsg = QString("Unknown subcommand: %1\n").arg(secondArg);
        usageMsg += QString("must be: clear");
        TclAppendResult(interp, qPrintable(usageMsg));
        return TCL_ERROR;
      }
    }

    uint index = 1;
    QStringList history{};
    for (QStringList::const_iterator it = console->history.cbegin();
         it != console->history.cend(); ++it) {
      QString cmd = *it;
      cmd.replace("\n", "\n\t");  // for multiline commands
      history.append(QString("%1\t%2").arg(index).arg(cmd));
      index++;
    }
    if (!history.isEmpty()) {
      TclAppendResult(interp, qPrintable(history.join("\n")));
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
      TclAppendResult(interp, qPrintable(usageMsg));
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
      TclAppendResult(interp, qPrintable(usageMsg));
      return TCL_ERROR;
    }
    // need to put it the event queue otherwise it will crash
    int methodIndex = console->metaObject()->indexOfMethod("clearText()");
    QMetaMethod method = console->metaObject()->method(methodIndex);
    method.invoke(console, Qt::QueuedConnection);
    return TCL_OK;
  };

  Tcl_CreateCommand(interp, "clear", clear_, this, nullptr);

  auto unknown = [](ClientData clientData, Tcl_Interp *interp, int argc,
                    const char *argv[]) {
    QStringList params;
    for (int i = 1; i < argc; ++i) params << argv[i];
    QProcess proc;
    bool started{false};
    Tcl_ResetResult(interp);
    QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&]() {
      const QByteArray data = proc.readAllStandardOutput();
      Tcl_AppendResult(interp, qPrintable(data), nullptr);
    });
    QObject::connect(&proc, &QProcess::readyReadStandardError, [&]() {
      const QByteArray data = proc.readAllStandardError();
      Tcl_AppendResult(interp, qPrintable(data), nullptr);
    });
    QObject::connect(&proc, &QProcess::started, [&]() { started = true; });
    startShellCommand(proc, params);
    proc.waitForFinished(-1);

    if (!started) {
      Tcl_AppendResult(
          interp,
          qPrintable(QString("invalid command name \"%1\"").arg(argv[1])),
          nullptr);
      return TCL_ERROR;
    }

    auto status = proc.exitStatus();
    auto exitCode = proc.exitCode();
    if (status == QProcess::NormalExit) {
      return (exitCode == 0) ? TCL_OK : TCL_ERROR;
    }
    return TCL_ERROR;
  };

  Tcl_CreateCommand(interp, "unknown", unknown, nullptr, nullptr);
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

bool TclConsoleWidget::hasOpenBracket(const QString &str) const {
  QStack<int> stack;
  for (const auto &ch : str) {
    if (ch == '{') stack.push(0);
    if (ch == '}') {
      if (stack.isEmpty())
        return false;
      else
        stack.pop();
    }
  }
  return !stack.isEmpty();
}

bool TclConsoleWidget::hasCloseBracket(const QString &str) const {
  for (auto iter = str.crbegin(); iter != str.crend(); iter++)
    if (*iter == QChar{'}'}) return true;
  return false;
}

void FOEDAG::TclConsoleWidget::startShellCommand(QProcess &process,
                                                 const QStringList &input) {
#if (defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__))
  QStringList params{input};
  const QString prog{params.takeFirst()};
  process.start(prog, params);
#else
  process.start("/bin/sh", QStringList() << "-c" << input.join(" "));
#endif
}

State TclConsoleWidget::state() const { return m_state; }

void TclConsoleWidget::setParsers(const std::vector<LineParser *> &parsers) {
  m_formatter.setParsers(parsers);
}

void TclConsoleWidget::addParser(LineParser *parser) {
  m_formatter.addParser(parser);
}

void TclConsoleWidget::setState(const State &state) {
  if (m_state != state) {
    m_state = state;
    emit stateChanged(m_state);
  }
}

}  // namespace FOEDAG
