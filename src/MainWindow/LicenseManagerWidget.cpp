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
#include "LicenseManagerWidget.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>

namespace FOEDAG {

LicenseManagerWidget::LicenseManagerWidget(const QString &path, QWidget *parent)
    : Dialog(parent), m_path(path) {
  QPushButton *btn = new QPushButton{"Copy license...", this};
  connect(btn, &QPushButton::clicked, this, &LicenseManagerWidget::selectFile);
  setWindowTitle("License manager");

  m_label = new QLabel{};
  updateLabel();

  QGroupBox *groupbox = new QGroupBox{this};
  groupbox->setTitle("Licenses");
  QGridLayout *layout = new QGridLayout;
  layout->addWidget(m_label);
  layout->addWidget(btn, 1, 0, Qt::AlignLeft);
  groupbox->setLayout(layout);

  QGridLayout *mainLayout = new QGridLayout;
  mainLayout->addWidget(groupbox);
  initDialogBox(mainLayout, Dialog::Ok);
  setLayout(mainLayout);
}

void LicenseManagerWidget::setLicensePath(const QString &path) {
  m_path = path;
  updateLabel();
}

void LicenseManagerWidget::selectFile() {
  QString path = solveSystemVars(m_path, this);
  if (path.isEmpty()) return;

  QFileInfo info{path};
  auto file = QFileDialog::getOpenFileName(this, "Select license file",
                                           QDir::homePath(),
                                           QString{"License file (*.*)"});
  if (!file.isEmpty()) {
    QDir dir{info.path()};
    if (!dir.exists()) {
      dir.mkpath(info.path());
    } else {
      QFile::remove(path);
    }
    if (info.isDir()) {
      // in case directory provided, take filename from selecetd file
      QFileInfo info{file};
      path = path + QDir::separator() + info.fileName();
    }
    if (!QFile::copy(file, path)) {
      QMessageBox::critical(
          this, "Copy license file",
          QString{"Failed to install license file %1"}.arg(file));
    } else {
      QMessageBox::information(
          this, "License installation",
          QString{"License %1 successfully installed"}.arg(file));
    }
  }
}

void LicenseManagerWidget::updateLabel() {
  const QFileInfo info{m_path};
  m_label->setText(QString{
      "Click 'Copy license' to copy a license file into the %1 directory"}
                       .arg(info.path()));
}

QString LicenseManagerWidget::solveSystemVars(const QString &p,
                                              QWidget *parent) {
  static const QRegularExpression env{"(\\$[^\\/]+)\\/?"};
  QString path{p};
  auto match = env.match(p);
  while (match.hasMatch()) {
    auto envVar = match.captured(1);
    auto envValue = qEnvironmentVariable(envVar.mid(1).toLatin1(), {});
    if (envValue.isEmpty()) {
      QMessageBox::critical(
          parent, "System environment variable",
          QString{"System environment variable %1 is empty"}.arg(envVar));
      return {};
    }
    path.replace(envVar, envValue);
    match = env.match(p, match.capturedEnd(1));
  }
  return path;
}

}  // namespace FOEDAG
