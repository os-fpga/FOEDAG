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
#include "EditorSettings.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

namespace FOEDAG {

EditorSettings::EditorSettings(QWidget* parent) : QDialog(parent) {
  QGridLayout* layout = new QGridLayout{};
  layout->addWidget(new QLabel{"Editor Name"}, 0, 1);
  layout->addWidget(new QLabel{"Command"}, 0, 2);
  for (int i = 0; i < 5; i++) {
    m_editors.push_back(std::make_pair(new QLineEdit{}, new QLineEdit{}));
    auto label = new QLabel{QString{"%1:"}.arg(QString::number(i + 1))};
    layout->addWidget(label, i + 1, 0);
    layout->addWidget(m_editors.back().first, i + 1, 1);
    layout->addWidget(m_editors.back().second, i + 1, 2);
  }
  auto buttons =
      new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  connect(buttons, &QDialogButtonBox::accepted, this, &EditorSettings::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &EditorSettings::reject);
  layout->addWidget(buttons, 6, 0, 1, 0, Qt::AlignRight);

  setLayout(layout);
  setWindowTitle("Editors settings");
}

std::vector<std::pair<QString, QString> > EditorSettings::editor() const {
  std::vector<std::pair<QString, QString> > e;
  for (const auto& line : m_editors) {
    e.push_back(std::make_pair(line.first->text().simplified(),
                               line.second->text().simplified()));
  }
  return e;
}

void EditorSettings::setEditor(const std::pair<QString, QString>& data,
                               int index) {
  if (index >= 0 && index < m_editors.size()) {
    m_editors[index].first->setText(data.first);
    m_editors[index].second->setText(data.second);
  }
}

}  // namespace FOEDAG
