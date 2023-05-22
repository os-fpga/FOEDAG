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
#include "PathEdit.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>

namespace FOEDAG {

PathEdit::PathEdit(QWidget *parent) : QWidget{parent} {
  auto layout = new QHBoxLayout{};

  m_edit = new QLineEdit{this};
  QToolButton *tool = new QToolButton{this};
  tool->setText("...");
  layout->addWidget(m_edit);
  layout->addWidget(tool);
  setLayout(layout);

  connect(tool, &QToolButton::clicked, this, [this]() {
    auto folder = QFileDialog::getExistingDirectory(
        this, tr("Select Directory"), text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks |
            QFileDialog::DontUseNativeDialog);
    if (!folder.isEmpty()) setText(folder);
  });
}

QString PathEdit::text() const { return m_edit->text(); }

void PathEdit::setText(const QString &text) { m_edit->setText(text); }

QLineEdit *PathEdit::lineEdit() { return m_edit; }

}  // namespace FOEDAG
