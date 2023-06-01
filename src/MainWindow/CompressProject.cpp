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
#include "CompressProject.h"

#include <QBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>

#include "MainWindow/PathEdit.h"
#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"

namespace FOEDAG {

CompressProject::CompressProject(const fs::path &project, QWidget *parent)
    : Dialog(parent), m_extension{".zip"}, m_projectPath(project) {
  QLineEdit *projectNameLine = new QLineEdit;
  projectNameLine->setObjectName("projectName");
  auto pathEdit = new PathEdit;
  pathEdit->layout()->setContentsMargins(0, 0, 0, 0);
  QLineEdit *projectPathLine = pathEdit->lineEdit();
  projectPathLine->setObjectName("projectPath");

  QGridLayout *grid = new QGridLayout;
  grid->addWidget(new QLabel{"File name:"}, 0, 0);
  grid->addWidget(new QLabel{"File path:"}, 1, 0);
  grid->addWidget(projectNameLine, 0, 1);
  grid->addWidget(pathEdit, 1, 1);
  QVBoxLayout *layout = new QVBoxLayout{};
  layout->addLayout(grid);

#ifndef _WIN32
  QGroupBox *groupBox = new QGroupBox("Select compress parameter");
  QRadioButton *radio1 = new QRadioButton("*.zip");
  radio1->setProperty("ext", ".zip");
  QRadioButton *radio2 = new QRadioButton("*.tar.gz");
  radio2->setProperty("ext", ".tar.gz");
  QHBoxLayout *hbox = new QHBoxLayout;
  hbox->addWidget(radio1);
  hbox->addWidget(radio2);
  hbox->addStretch(1);
  groupBox->setLayout(hbox);
  layout->addWidget(groupBox);
  connect(radio1, &QRadioButton::toggled, this,
          &CompressProject::extensionHasChanged);
  connect(radio2, &QRadioButton::toggled, this,
          &CompressProject::extensionHasChanged);
  radio1->setChecked(true);
#endif

  initDialogBox(layout, Dialog::Ok | Dialog::Cancel);
  setLayout(layout);
  setWindowTitle("Save Diagnostics");

  auto projectName = project.filename();
  projectNameLine->setText(QString::fromStdString(projectName.string()));
  projectNameLine->selectAll();
  projectPathLine->setText(
      QString::fromStdString(project.parent_path().string()));
  connect(this, &CompressProject::accepted, this,
          &CompressProject::compressProject);
}

void CompressProject::compressProject() {
  auto projectNameLine = findChild<QLineEdit *>("projectName");
  auto projectPathLine = findChild<QLineEdit *>("projectPath");
  if (projectNameLine && projectPathLine) {
    auto projectName = projectNameLine->text().toStdString();
    auto projectPath = projectPathLine->text().toStdString();
    auto originalProjectPath = m_projectPath.parent_path().string();
    if (m_extension == ".tar.gz") {
      // tar -czvf file.tar.gz directory
      StringVector args{"-czvf", projectName + m_extension.toStdString(), "-C",
                        originalProjectPath, m_projectPath.filename().string()};
      ExecuteSystemCommand("tar", args, projectPath);
    } else {
      // zip -r foo.zip foo
      StringVector args{"-r", projectName + m_extension.toStdString(),
                        m_projectPath.filename().string()};
      if (ExecuteSystemCommand("zip", args, originalProjectPath)) {
        // since zip file is created within original project path we have to
        // move it to the path user selected. zip doesn't allow to drop parent
        // folder structure.
        if (projectPath != originalProjectPath) {
          args = {projectName + m_extension.toStdString(), projectPath};
          ExecuteSystemCommand("mv", args, originalProjectPath);
        }
      }
    }
  }
}

void CompressProject::extensionHasChanged(bool checked) {
  if (checked) m_extension = sender()->property("ext").toString();
}

bool CompressProject::ExecuteSystemCommand(const std::string &command,
                                           const std::vector<std::string> &args,
                                           const std::string &workingDir) {
  std::ostringstream help;
  auto res =
      FileUtils::ExecuteSystemCommand(command, args, &help, -1, workingDir);
  if (res.code != 0) {
    QMessageBox::critical(this, "Compression error",
                          QString::fromStdString(help.str()));
  }
  return res.code == 0;
}

}  // namespace FOEDAG
