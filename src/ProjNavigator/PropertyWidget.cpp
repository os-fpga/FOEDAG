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
#include "PropertyWidget.h"

#include <QBoxLayout>
#include <QDir>

namespace FOEDAG {

PropertyWidget::PropertyWidget(ProjectManager *projectManager, QObject *parent)
    : m_projectManager(projectManager), QObject(parent) {
  m_parent = new QWidget;
  m_parent->setMinimumHeight(50);
  m_textEdit = new QTextEdit;
  m_textEdit->setReadOnly(true);
  QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom);
  layout->addWidget(m_textEdit);
  m_parent->setLayout(layout);
  auto margin = layout->contentsMargins();
  margin.setBottom(1);
  layout->setContentsMargins(margin);
}

QWidget *PropertyWidget::Widget() { return m_parent; }

void PropertyWidget::ShowProperty(const QString &file) {
  const QString filename =
      file.right(file.size() - (file.lastIndexOf(QDir::separator()) + 1));
  QString path = file.left(file.lastIndexOf("/") + 1);
  path.replace(PROJECT_OSRCDIR, m_projectManager->getProjectPath());
  QString text = QString("<b>%1:</b><br>------------------<br><b>Path:</b> %2")
                     .arg(filename, path);
  m_textEdit->setText(text);
  m_parent->show();
}

}  // namespace FOEDAG
