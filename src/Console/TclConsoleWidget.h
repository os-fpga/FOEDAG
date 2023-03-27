#pragma once

#include <QPlainTextEdit>
#include <QTextBlock>
#include <memory>
#include <ostream>

#include "ConsoleDefines.h"
#include "ConsoleInterface.h"
#include "OutputFormatter.h"
#include "QConsole/qconsole.h"

class QProcess;
namespace FOEDAG {

enum class State {
  IDLE,
  IN_PROGRESS,
};

struct ErrorInfo {
  QString file;
  QString line;
};

class TclConsoleBuffer;
class TclConsoleWidget : public QConsole {
  Q_OBJECT
 public:
  explicit TclConsoleWidget(TclInterp *interp,
                            std::unique_ptr<ConsoleInterface> iConsole,
                            TclConsoleBuffer *buffer,
                            QWidget *parent = nullptr);
  bool isRunning() const override;
  QString getPrompt() const;
  TclConsoleBuffer *getBuffer();
  TclConsoleBuffer *getErrorBuffer();
  static const char *consoleObjectName();

  State state() const;

  /*!
   * \brief setParsers. Console will take ownership of the parsers
   */
  void setParsers(const std::vector<LineParser *> &parsers);
  /*!
   * \brief addParser. Add parser to the list and take ownership of the pointer
   */
  void addParser(LineParser *parser);

 public slots:
  void clearText();
  void showPrompt();

 signals:
  void searchEnable();
  void stateChanged(FOEDAG::State);
  void linkActivated(const FOEDAG::ErrorInfo &);

 protected:
  QString interpretCommand(const QString &command, int *res) override;
  QStringList suggestCommand(const QString &cmd, QString &prefix) override;
  bool isCommandComplete(const QString &command) override;
  void handleSearch() override;
  void handleTerminateCommand() override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;

 private slots:
  void put(const QString &str) override;
  void putError(const QString &str);
  void commandDone();

 private:
  void putMessage(const QString &message, OutputFormat format);
  void setState(const State &state);
  void handleLink(const QPoint &p);
  void registerCommands(TclInterp *interp);
  bool hasPrompt() const;
  bool handleCommandFromHistory(const QString &command,
                                QString &commandFromHist);

  bool hasOpenBracket(const QString &str) const;
  bool hasCloseBracket(const QString &str) const;

  static void startShellCommand(QProcess &process, const QStringList &input);

 private:
  std::unique_ptr<ConsoleInterface> m_console;
  TclConsoleBuffer *m_buffer;
  TclConsoleBuffer *m_errorBuffer;
  State m_state = State::IDLE;

  bool m_linkActivated{true};
  Qt::MouseButton m_mouseButtonPressed{Qt::NoButton};
  OutputFormatter m_formatter;
};

}  // namespace FOEDAG

Q_DECLARE_METATYPE(FOEDAG::ErrorInfo)
