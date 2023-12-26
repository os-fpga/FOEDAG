/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include "WelcomePageWidget.h"

#include <QAction>
#include <QCheckBox>
#include <QFile>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>

#include "ui_WelcomePageWidget.h"

using namespace FOEDAG;

namespace {
static const auto ETC_DIR = "etc";
static const auto WELCOME_PAGE_DIR = "Welcome_Page";
static const auto LOGO_FILENAME = "WelcomeLogo.png";
static const auto DESCRIPTION_FILENAME = "WelcomeDescription.txt";
static const auto COPY_FILENAME = "copyrightDescription.txt";
}  // namespace

WelcomePageWidget::WelcomePageWidget(const QString &header,
                                     const std::filesystem::path &sourcesPath,
                                     QWidget *parent)
    : QWidget(parent), ui(new Ui::WelcomePageWidget) {
  ui->setupUi(this);
  std::filesystem::path srcDir = sourcesPath / ETC_DIR / WELCOME_PAGE_DIR;
  ui->labelHeader->setText(getFileContent(srcDir / DESCRIPTION_FILENAME));
  ui->labelBottom->setText(getFileContent(srcDir / COPY_FILENAME));

  std::filesystem::path labelPath = srcDir / LOGO_FILENAME;
  auto logoPixmap = QPixmap(QString::fromStdString(labelPath.string()));
  if (!logoPixmap.isNull()) {
    logoPixmap = logoPixmap.scaledToWidth(500, Qt::SmoothTransformation);
    ui->labelLogo->setPixmap(logoPixmap);
  } else {
    ui->labelLogo->hide();
  }

  ui->checkBox->setCheckState(Qt::Checked);
  connect(ui->checkBox, &QAbstractButton::clicked, this,
          [this]() { emit welcomePageClosed(true); });

  ui->groupBox->hide();

  // Background setup
  auto defaultPalette = palette();
  defaultPalette.setColor(QPalette::Window, Qt::white);
  setPalette(defaultPalette);
  setAutoFillBackground(true);
}

void WelcomePageWidget::addAction(QAction &act) {
  auto actionButton = createActionButton(act);
  connect(actionButton, &QPushButton::clicked, &act, &QAction::triggered);
  ui->verticalLayout->addWidget(actionButton, 0, Qt::AlignLeft | Qt::AlignTop);
}

void WelcomePageWidget::addRecentProject(QAction &act) {
  auto buttons = createActionButton(act);
  buttons->setIcon(QIcon{":/images/dot.png"});
  connect(buttons, &QPushButton::clicked, &act, &QAction::triggered);
  ui->verticalLayoutRecent->addWidget(buttons, 0, Qt::AlignLeft | Qt::AlignTop);
  ui->groupBox->show();
  ui->line->setMinimumHeight(130 + ui->verticalLayoutRecent->count() * 25);
}

void WelcomePageWidget::setConsoleWidget(QWidget *console) {
  if (console) {
    ui->consoleWidget->setLayout(new QVBoxLayout);
    ui->consoleWidget->layout()->setContentsMargins(0, 0, 0, 0);
    ui->consoleWidget->layout()->addWidget(new QLabel{"Console"});
    ui->consoleWidget->layout()->addWidget(console);
    ui->consoleWidget->setMaximumHeight(150);
  }
}

QPushButton *WelcomePageWidget::createActionButton(QAction &action) {
  auto btn = new QPushButton(action.text());
  btn->setIcon(action.icon());
  btn->setFlat(true);
  btn->setCursor(Qt::PointingHandCursor);

  btn->setStyleSheet(
      "QPushButton:hover { border: none; text-decoration: "
      "underline;} QPushButton { text-align:left;} ");

  return btn;
}

QString WelcomePageWidget::getFileContent(
    const std::filesystem::path &srcDir) const {
  auto result = QString{};

  auto descFile = QFile(QString::fromStdString(srcDir.string()));
  if (!descFile.open(QIODevice::ReadOnly)) return result;

  return descFile.readAll().trimmed();
}

void WelcomePageWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) emit welcomePageClosed(false);
}
