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
class QLabel;

namespace FOEDAG {

class LicenseManagerWidget : public QDialog {
 public:
  explicit LicenseManagerWidget(const QString &path, QWidget *parent = nullptr,
                                Qt::WindowFlags f = Qt::WindowFlags());

  void setLicensePath(const QString &path);

 private slots:
  void selectFile();

 private:
  void updateLabel();
  static QString solveSystemVars(const QString &p, QWidget *parent);

 private:
  QString m_path;
  QLabel *m_label{};
};

}  // namespace FOEDAG
