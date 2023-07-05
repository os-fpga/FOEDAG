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
#include "ProgrammerSettingsWidget.h"

#include <QSettings>

#include "qdebug.h"
#include "ui_ProgrammerSettingsWidget.h"

namespace FOEDAG {

const char *ProgrammerSettingsWidget::ALWAYS_VERIFY{"always-verify"};

ProgrammerSettingsWidget::ProgrammerSettingsWidget(QSettings &settings,
                                                   QWidget *parent)
    : Dialog(parent),
      ui(new Ui::ProgrammerSettingsWidget),
      m_settings(settings) {
  ui->setupUi(this);
  initDialogBox(ui->gridLayout, Dialog::Ok | Dialog::Cancel);
  ui->checkBoxAlwaysPerformVerify->setChecked(
      m_settings.value(ALWAYS_VERIFY, false).toBool());
  connect(this, &ProgrammerSettingsWidget::accepted, this,
          &ProgrammerSettingsWidget::apply);
}

ProgrammerSettingsWidget::~ProgrammerSettingsWidget() { delete ui; }

void ProgrammerSettingsWidget::openTab(int index) {
  if (index >= 0 && index < ui->tabWidget->count())
    ui->tabWidget->setCurrentIndex(index);
}

void ProgrammerSettingsWidget::apply() {
  m_settings.setValue(ALWAYS_VERIFY,
                      ui->checkBoxAlwaysPerformVerify->isChecked());
}

}  // namespace FOEDAG
