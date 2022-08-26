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

#include "welcomepagewidget.h"

#include <QAction>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

using namespace FOEDAG;

namespace {
static constexpr auto HEADER_MARGIN = 6;
static constexpr auto HEADER_POINTSIZE = 24;
static constexpr auto DESCRIPTION_POINTSIZE = 14;
static constexpr auto PAGE_MARGIN = 30;
static constexpr auto PAGE_SPACING = 20;

static const auto LOGO_PATH = QString("/etc/welcome_page/Logo.png");
static const auto DESCRIPTION_PATH =
    QString("/etc/welcome_page/WelcomeDescription.txt");
}  // namespace

WelcomePageWidget::WelcomePageWidget(const QString &header,
                                     const QString &sourcesPath,
                                     QWidget *parent)
    : QWidget(parent), m_actionsLayout(new QVBoxLayout()) {
  // Header label
  auto headerLabel = new QLabel(header.toUpper(), this);
  auto headerFont = headerLabel->font();
  headerFont.setPointSize(HEADER_POINTSIZE);
  headerLabel->setFont(headerFont);
  headerLabel->setContentsMargins(HEADER_MARGIN, HEADER_MARGIN, HEADER_MARGIN,
                                  0);

  // Description label
  auto descLabel = new QLabel(getDescription(sourcesPath), this);
  auto descFont = descLabel->font();
  descFont.setPointSize(DESCRIPTION_POINTSIZE);
  descLabel->setFont(descFont);
  descLabel->setContentsMargins(HEADER_MARGIN, 0, HEADER_MARGIN,
                                HEADER_MARGIN * 2);

  // Group box with start actions
  auto quickStartGroupBox = new QGroupBox(this);
  auto groupBoxLayout = new QGridLayout();
  m_actionsLayout->addWidget(headerLabel);
  m_actionsLayout->addWidget(descLabel);
  groupBoxLayout->addLayout(m_actionsLayout, 0, 0, Qt::AlignCenter);
  quickStartGroupBox->setLayout(groupBoxLayout);

  // Logo label
  auto logoLabel = new QLabel(this);
  logoLabel->setPixmap(QPixmap(sourcesPath + LOGO_PATH));

  // Main layout
  auto mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(PAGE_SPACING);
  mainLayout->addWidget(logoLabel, 0, Qt::AlignRight | Qt::AlignTop);
  mainLayout->addWidget(quickStartGroupBox);
  mainLayout->addStretch(1);
  mainLayout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN,
                                 PAGE_MARGIN);

  // Background setup
  auto defaultPalette = palette();
  defaultPalette.setColor(QPalette::Window, Qt::white);
  setPalette(defaultPalette);
  setAutoFillBackground(true);
}

void WelcomePageWidget::addAction(QAction &act) {
  auto actionButton = createActionButton();
  connect(actionButton, &QPushButton::clicked, [&act]() { act.trigger(); });
  actionButton->setText(act.text());
  m_actionsLayout->addWidget(actionButton, 0, Qt::AlignLeft | Qt::AlignTop);
}

QPushButton *WelcomePageWidget::createActionButton() {
  auto btn = new QPushButton();
  btn->setFlat(true);
  btn->setCursor(Qt::PointingHandCursor);

  btn->setStyleSheet(
      "QPushButton:hover { background-color: transparent; text-decoration: "
      "underline;}");

  return btn;
}

QString WelcomePageWidget::getDescription(const QString &sourcesPath) const {
  auto result = QString{};

  auto descFile = QFile(sourcesPath + DESCRIPTION_PATH);
  if (!descFile.open(QIODevice::ReadOnly)) return result;

  auto in = QTextStream(&descFile);
  result = in.readAll();
  descFile.close();

  return result.trimmed();
}
