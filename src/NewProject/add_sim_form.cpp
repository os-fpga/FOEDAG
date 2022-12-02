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
#include "add_sim_form.h"

#include <QFileInfo>

#include "ProjectManager/project_manager.h"
#include "ui_add_sim_form.h"

namespace FOEDAG {

addSimForm::addSimForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::addSimForm) {
  ui->setupUi(this);

  m_widgetGrid = new sourceGrid(ui->m_frame);
  m_widgetGrid->setGridType(GT_SIM);
  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom, ui->m_frame);
  box->addWidget(m_widgetGrid);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  ui->m_frame->setLayout(box);

  ui->m_ckkBoxCopy->setText(tr("Copy sources into project. "));
  ui->m_ckkBoxCopy->setCheckState(Qt::CheckState::Unchecked);
}

addSimForm::~addSimForm() { delete ui; }

QList<filedata> addSimForm::getFileData() const {
  return m_widgetGrid->getTableViewData();
}

bool addSimForm::IsCopySource() const { return ui->m_ckkBoxCopy->isChecked(); }

void addSimForm::updateUi(ProjectManager *pm) {
  if (!pm) return;

  m_widgetGrid->ClearTable();
  auto libs = pm->SimulationLibraries();
  int index{0};
  for (const auto &lang_file : pm->SimulationFiles()) {
    filedata data;
    data.m_language = lang_file.first.language;
    data.m_groupName = lang_file.first.group;
    if (index < libs.size()) {
      if (!libs.at(index).first.empty()) {
        if (!libs.at(index).second.empty())
          data.m_workLibrary =
              QString::fromStdString(libs.at(index).second.front());
      }
    }
    const QStringList fileList =
        QString::fromStdString(lang_file.second).split(" ");
    for (const auto &file : fileList) {
      const QFileInfo info{file};
      data.m_fileName = info.fileName();
      data.m_filePath = info.path();
      data.m_isFolder = false;
      data.m_fileType = info.suffix();
      m_widgetGrid->AddTableItem(data);
    }
    index++;
  }
}

void addSimForm::SetTitle(const QString &title) {
  ui->m_labelTitle->setText(title);
}

}  // namespace FOEDAG
