#pragma once

#include "ConsoleInterface.h"

class TclConsole : public ConsoleInterface {
 public:
  TclConsole(QObject *parent = nullptr);
  void run(const QString &command) override;
};
