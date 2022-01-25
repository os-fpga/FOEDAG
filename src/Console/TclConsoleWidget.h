#pragma once

#include <QPlainTextEdit>
#include <QTextBlock>
#include <memory>
#include <ostream>

#include "ConsoleInterface.h"
#include "QConsole/qconsole.h"
#include "Tcl/TclInterpreter.h"

class StreamBuffer;
class TclConsoleWidget : public QConsole {
  Q_OBJECT
 public:
  explicit TclConsoleWidget(Tcl_Interp *interp,
                            std::unique_ptr<ConsoleInterface> iConsole,
                            StreamBuffer *buffer, QWidget *parent = nullptr);
  bool isRunning() const override;

 public slots:
  void clearText();

 signals:
  void searchEnable();
  void sendCommand(QString);
  void abort();

 protected:
  QString interpretCommand(const QString &command, int *res) override;
  QStringList suggestCommand(const QString &cmd, QString &prefix) override;
  bool isCommandComplete(const QString &command) override;
  void handleSearch() override;

 private slots:
  void put(const QString &str);
  void commandDone();

 private:
  void updateScroll();
  void handleLink(const QPoint &p);
  void registerCommands(Tcl_Interp *interp);
  bool hasPrompt() const;
  bool handleCommandFromHistory(const QString &command,
                                QString &commandFromHist);

 private:
  std::unique_ptr<ConsoleInterface> m_console;
  StreamBuffer *m_buffer;

  bool m_linkActivated{false};
  bool m_command_done{true};
};
