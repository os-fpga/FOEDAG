#pragma once

#include <QElapsedTimer>
#include <QWidget>

#include "../Compiler/Compiler.h"
#include "client/GateIO.h"

class NCriticalPathWidget : public QWidget {
  Q_OBJECT

 public:
  explicit NCriticalPathWidget(FOEDAG::Compiler*, QWidget* parent = nullptr);
  ~NCriticalPathWidget();

 private slots:
  void onFlatRoutingOnDetected();
  void requestPathList(const QString& initiator);

 private:
  bool m_prevIsFlatRoutingFlag = false;

  class NCriticalPathView* m_view = nullptr;
  class NCriticalPathToolsWidget* m_toolsWidget = nullptr;
  class NCriticalPathStatusBar* m_statusBar = nullptr;

  client::GateIO m_gateIO;

  QElapsedTimer m_fetchPathListTimer;

  void notifyError(QString, QString);
};
