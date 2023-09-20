/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <QMap>
#include <QVector>
#include <QWidget>

#include "MainWindow/Dialog.h"
#include "ProgrammerGuiCommon.h"

namespace Ui {
class ProgrammerSettingsWidget;
}

class QSettings;
namespace FOEDAG {

struct ProgrammerSettings {
  QMap<QString, uint64_t> frequency;
  QVector<DeviceInfo *> devices;
};

class ProgrammerSettingsWidget : public Dialog {
  Q_OBJECT

 public:
  explicit ProgrammerSettingsWidget(const ProgrammerSettings &pSettings,
                                    QSettings &settings,
                                    QWidget *parent = nullptr);
  ~ProgrammerSettingsWidget() override;

  void openTab(int index);

 private slots:
  void apply();

 private:
  template <typename T>
  QString ToHexString(T val) {
    return QString{"0x%1"}.arg(val, 8, 16, QLatin1Char{'0'});
  }

 private:
  Ui::ProgrammerSettingsWidget *ui;
  QSettings &m_settings;
};

}  // namespace FOEDAG
