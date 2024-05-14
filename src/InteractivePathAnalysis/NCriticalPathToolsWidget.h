/**
  * @file NCriticalPathToolsWidget.h
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

#include <QWidget>

#include "../Compiler/Compiler.h"
#include "NCriticalPathParameters.h"
#include "Process.h"

class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class QFormLayout;

namespace FOEDAG {

class CustomMenu;
class RefreshIndicatorButton;

class NCriticalPathToolsWidget : public QWidget {
  Q_OBJECT
 public:
  NCriticalPathToolsWidget(FOEDAG::Compiler*,
                           const std::filesystem::path& settingsFilePath,
                           QWidget* parent = nullptr);
  ~NCriticalPathToolsWidget() = default;

  void resetConfigurationUI();

  NCriticalPathParametersPtr parameters() const { return m_parameters; }
  void refreshCritPathContextOnSettingsChanged();

 public slots:
  void onConnectionStatusChanged(bool);
  void tryRunPnRView();
  void deactivatePlaceAndRouteViewProcess();
  void enablePlaceAndRouteViewButton();

 signals:
  void pathListRequested(const QString&);
  void PnRViewRunStatusChanged(bool);
  void highLightModeChanged();
  void isFlatRoutingOnDetected();
  void isFlatRoutingOffDetected();
  void vprProcessErrorOccured(QString);
  void serverPortNumDetected(int);

 private:
  FOEDAG::Compiler* m_compiler = nullptr;

  QLineEdit* m_leNCriticalPathNum = nullptr;
  QComboBox* m_cbHighlightMode = nullptr;
  QCheckBox* m_cbDrawCritPathContour = nullptr;
  QComboBox* m_cbPathType = nullptr;
  QComboBox* m_cbDetail = nullptr;
  QCheckBox* m_cbIsFlatRouting = nullptr;
  QCheckBox* m_cbIsLogToFileEnabled = nullptr;

  Process m_vprProcess;

  NCriticalPathParametersPtr m_parameters;

  CustomMenu* m_pathsOptionsMenu = nullptr;

  QPushButton* m_bnRunPnRView = nullptr;

  void setupCriticalPathsOptionsMenu(QPushButton*);

  QString projectLocation();
  QString vprBaseCommand();

  void addRowToFormLayout(QFormLayout* formLayout, const QString& labelText,
                          QWidget* widget) const;
};

}  // namespace FOEDAG
