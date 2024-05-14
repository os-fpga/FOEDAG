/**
  * @file NCriticalPathWidget.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QElapsedTimer>
#include <QWidget>

#include "../Compiler/Compiler.h"
#include "client/GateIO.h"

namespace FOEDAG {

class NCriticalPathWidget : public QWidget {
  Q_OBJECT

 public:
  explicit NCriticalPathWidget(
      FOEDAG::Compiler*, const QString& logFilePath,
      const std::filesystem::path& settingsFilePath = "",
      QWidget* parent = nullptr);
  ~NCriticalPathWidget();

 private slots:
  void onFlatRoutingOnDetected();
  void onFlatRoutingOffDetected();
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

}  // namespace FOEDAG
