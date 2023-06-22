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
#include <QTextStream>
#include <QVBoxLayout>

using namespace FOEDAG;

namespace {
static constexpr auto HEADER_MARGIN = 6;
static constexpr auto HEADER_POINTSIZE = 24;
static constexpr auto DESCRIPTION_POINTSIZE = 14;
static constexpr auto PAGE_MARGIN = 30;
static constexpr auto PAGE_SPACING = 20;

static const auto ETC_DIR = "etc";
static const auto WELCOME_PAGE_DIR = "Welcome_Page";
static const auto LOGO_FILENAME = "quickLogic-logo-w-reg-mark.png";
static const auto DESCRIPTION_FILENAME = "WelcomeDescription.txt";
}  // namespace

WelcomePageWidget::WelcomePageWidget(const QString &header,
                                     const std::filesystem::path &sourcesPath,
                                     QWidget *parent)
    : QWidget(parent),
      m_actionsLayout(new QVBoxLayout()),
      m_recentProjectsLayout(new QVBoxLayout()) {
  std::filesystem::path srcDir = sourcesPath / ".." / "share" / "aurora" / ETC_DIR / WELCOME_PAGE_DIR;

  // Header label
  auto headerLabel = new QLabel(header.toUpper(), this);
  auto headerFont = headerLabel->font();
  headerFont.setPointSize(HEADER_POINTSIZE);
  headerLabel->setFont(headerFont);
  headerLabel->setContentsMargins(0, HEADER_MARGIN, HEADER_MARGIN, 0);

  // Description label
  auto descLabel = new QLabel(getDescription(srcDir), this);
  auto descFont = descLabel->font();
  descFont.setPointSize(DESCRIPTION_POINTSIZE);
  descLabel->setFont(descFont);
  descLabel->setContentsMargins(0, 0, HEADER_MARGIN, HEADER_MARGIN * 2);

  // Group box with start actions
  auto quickStartGroupBox = new QGroupBox(this);
  m_actionsLayout->addWidget(headerLabel);
  m_actionsLayout->addWidget(descLabel);
  quickStartGroupBox->setLayout(m_actionsLayout);

  // Logo label
  auto logoLabel = new QLabel(this);
  std::filesystem::path labelPath = sourcesPath / ".." / "share" / "aurora" / ETC_DIR / LOGO_FILENAME;
  auto logoPixmap = QPixmap(QString::fromStdString(labelPath.string()));
  if (!logoPixmap.isNull()) logoLabel->setPixmap(logoPixmap);

  // Main layout
  auto mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(PAGE_SPACING);
  mainLayout->addWidget(logoLabel, 0, Qt::AlignHCenter | Qt::AlignTop);
  mainLayout->addWidget(quickStartGroupBox);
  mainLayout->setContentsMargins(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN,
                                 PAGE_MARGIN);

  auto showPageCheckBox = new QCheckBox(tr("Show welcome page"), this);
  showPageCheckBox->setCheckState(Qt::Checked);
  connect(showPageCheckBox, &QAbstractButton::clicked,
          [this]() { emit welcomePageClosed(true); });
  mainLayout->addWidget(showPageCheckBox, 0, Qt::AlignHCenter);
  mainLayout->addStretch(1);
  m_mainLayout = mainLayout;

  // Background setup
  auto defaultPalette = palette();
  defaultPalette.setColor(QPalette::Window, Qt::white);
  setPalette(defaultPalette);
  setAutoFillBackground(true);
}

void WelcomePageWidget::addAction(QAction &act) {
  auto actionButton = createActionButton(act.text());
  connect(actionButton, &QPushButton::clicked, [&act]() { act.trigger(); });
  m_actionsLayout->addWidget(actionButton, 0, Qt::AlignLeft | Qt::AlignTop);
}

void WelcomePageWidget::addRecentProject(QAction &act) {
  initRecentProjects();
  auto buttons = createActionButton(act.text());
  connect(buttons, &QPushButton::clicked, this, [&act]() { act.trigger(); });
  m_recentProjectsLayout->addWidget(buttons, 0, Qt::AlignLeft | Qt::AlignTop);
}

QPushButton *WelcomePageWidget::createActionButton(const QString &text) {
  auto btn = new QPushButton(text);
  btn->setFlat(true);
  btn->setCursor(Qt::PointingHandCursor);

  btn->setStyleSheet(
      "QPushButton:hover { border: none; text-decoration: "
      "underline;} QPushButton { text-align:left;} ");

  return btn;
}

QString WelcomePageWidget::getDescription(
    const std::filesystem::path &srcDir) const {
  auto result = QString{};

  std::filesystem::path welcomeDescPath = srcDir / DESCRIPTION_FILENAME;
  auto descFile = QFile(QString::fromStdString(welcomeDescPath.string()));
  if (!descFile.open(QIODevice::ReadOnly)) return result;

  auto in = QTextStream(&descFile);
  result = in.readAll();
  descFile.close();

  return result.trimmed();
}

void WelcomePageWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) emit welcomePageClosed(false);
}

void WelcomePageWidget::initRecentProjects() {
  if (m_recentProjectsLayout->count() != 0) return;
  auto recentLabel = new QLabel("Recent Projects", this);
  auto recentFont = recentLabel->font();
  recentFont.setPointSize(DESCRIPTION_POINTSIZE);
  recentLabel->setFont(recentFont);
  recentLabel->setContentsMargins(0, 0, HEADER_MARGIN, HEADER_MARGIN * 2);
  m_recentProjectsLayout->addWidget(recentLabel);
  auto recentProjectsGroupBox = new QGroupBox(this);
  recentProjectsGroupBox->setLayout(m_recentProjectsLayout);
  m_mainLayout->insertWidget(2, recentProjectsGroupBox);
}
