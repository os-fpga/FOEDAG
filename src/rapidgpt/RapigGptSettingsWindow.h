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

#include <QDialog>
#include <QSettings>

class QLineEdit;
class QComboBox;

namespace Ui {
class RapigGptSettingsWindow;
}

namespace FOEDAG {

struct RapidGptSettings {
  QString key;
  QString precision;
  QString interactive;
  QString remoteUrl;
};

class RapigGptSettingsWindow : public QDialog {
  Q_OBJECT

 public:
  explicit RapigGptSettingsWindow(QSettings &settings,
                                  QWidget *parent = nullptr);

  static RapidGptSettings fromSettings(const QSettings &settings);

 private slots:
  void accept() override;

 private:
  Ui::RapigGptSettingsWindow *ui;
  QSettings &m_settings;
};

}  // namespace FOEDAG
