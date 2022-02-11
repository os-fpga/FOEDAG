#pragma once

#include <QPlainTextEdit>
#include <QTextBlock>
#include <memory>
#include <ostream>

#include "ConsoleDefines.h"
#include "ConsoleInterface.h"
#include "QConsole/qconsole.h"

namespace FOEDAG {

enum class State {
  IDLE,
  IN_PROGRESS,
};

class StreamBuffer;
class TclConsoleWidget : public QConsole {
  Q_OBJECT
 public:
  explicit TclConsoleWidget(TclInterp *interp,
                            std::unique_ptr<ConsoleInterface> iConsole,
                            StreamBuffer *buffer, QWidget *parent = nullptr);
  bool isRunning() const override;
  QString getPrompt() const;

  State state() const;

 public slots:
  void clearText();

 signals:
  void searchEnable();
  void stateChanged(FOEDAG::State);
  void linkActivated(const QString &);

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
  void put(const QString &str);
  void commandDone();

 private:
  void setState(const State &state);
  void handleLink(const QPoint &p);
  void registerCommands(TclInterp *interp);
  bool hasPrompt() const;
  bool handleCommandFromHistory(const QString &command,
                                QString &commandFromHist);

  bool hasOpenBracket(const QString &str) const;
  bool hasCloseBracket(const QString &str) const;

 private:
  std::unique_ptr<ConsoleInterface> m_console;
  StreamBuffer *m_buffer;
  State m_state = State::IDLE;

  bool m_linkActivated{false};
  Qt::MouseButton m_mouseButtonPressed{Qt::NoButton};
};

}  // namespace FOEDAG
