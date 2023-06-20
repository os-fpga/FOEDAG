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

#include "ProgrammerMain.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>

#include "ui_ProgrammerMain.h"

namespace FOEDAG {

QMenu *contextMenu = nullptr;

ProgrammerMain::ProgrammerMain(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ProgrammerMain) {
  ui->setupUi(this);

  contextMenu = new QMenu(this);
  contextMenu->addAction(ui->actionAdd_File);
  contextMenu->addAction(ui->actionReset);
  contextMenu->addSeparator();
  // contextMenu->addAction(ui->actionConfigure);
  contextMenu->addAction(ui->actionProgram);
  contextMenu->addAction(ui->actionVerify);
  contextMenu->addAction(ui->actionErase);
  contextMenu->addAction(ui->actionBlankcheck);

  QComboBox *hardware = new QComboBox;
  hardware->addItem("FTDI");
  hardware->setFixedWidth(120);

  QLabel *label1 = new QLabel("Hardware:");
  label1->setBuddy(hardware);

  QComboBox *iface = new QComboBox;
  iface->addItem("JTAG");
  iface->setFixedWidth(120);

  QLabel *label2 = new QLabel("Interface:");
  label2->setBuddy(iface);

  QProgressBar *progressBar = new QProgressBar;
  progressBar->setValue(80);
  progressBar->setAlignment(Qt::AlignHCenter);

  ui->toolBar->insertWidget(ui->actionDetect, label1);
  ui->toolBar->insertWidget(ui->actionDetect, hardware);
  ui->toolBar->insertWidget(ui->actionDetect, label2);
  ui->toolBar->insertWidget(ui->actionDetect, iface);
  ui->toolBar->addWidget(progressBar);
  ui->toolBar->layout()->setSpacing(5);
  ui->toolBar->layout()->setContentsMargins(5, 5, 5, 5);

  ui->treeWidget->header()->resizeSection(0, 150);
  ui->treeWidget->header()->resizeSection(1, 350);
  ui->treeWidget->header()->resizeSection(2, 200);
  ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  ui->treeWidget->expandAll();
  connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
          SLOT(onCustomContextMenu(QPoint)));
  connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
}

ProgrammerMain::~ProgrammerMain() { delete ui; }

void ProgrammerMain::onCustomContextMenu(const QPoint &point) {
  QModelIndex index = ui->treeWidget->indexAt(point);
  if (index.isValid()) {
    contextMenu->exec(ui->treeWidget->viewport()->mapToGlobal(point));
    // qDebug() << "hello";
  }
}

}  // namespace FOEDAG
