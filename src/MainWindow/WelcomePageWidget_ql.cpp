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
static constexpr auto DESCRIPTION_POINTSIZE = 18;
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

  // Group box with start actions
  auto quickStartGroupBox = new QGroupBox(this);
  quickStartGroupBox->setLayout(m_actionsLayout);


  // headerLayout:
  // headerLeftLayout (Aurora) - headerRightLayout (Quicklogic)
  QHBoxLayout* headerLayout = new QHBoxLayout();

  //================================================ HEADER Left : Tool Logo-Name-Description++
  QVBoxLayout* headerLeftLayout = new QVBoxLayout();
  
  QHBoxLayout* headerLeftAuroraLayout = new QHBoxLayout();

  QLabel* headerIconLabel = new QLabel(this);
  std::filesystem::path headerIconPath = sourcesPath / ".." / "share" / "aurora" / "etc" / "aurora_logo.svg";
  auto headerIconPixmap = QIcon(QString::fromStdString(headerIconPath.string())).pixmap(QSize(84,84));
  if (!headerIconPixmap.isNull()) headerIconLabel->setPixmap(headerIconPixmap);
  headerLeftAuroraLayout->addWidget(headerIconLabel);
  
  auto headerLabel = new QLabel(this);
  std::filesystem::path headerPath = sourcesPath / ".." / "share" / "aurora" / "etc" / "aurora_title.svg";
  auto headerPixmap = QIcon(QString::fromStdString(headerPath.string())).pixmap(QSize(524,77));
  if (!headerPixmap.isNull()) headerLabel->setPixmap(headerPixmap);
  headerLeftAuroraLayout->addWidget(headerLabel);
  headerLeftAuroraLayout->addStretch();
  
  QHBoxLayout* headeLeftDescriptionLayout = new QHBoxLayout();
    // Description label
  auto descLabel = new QLabel(getDescription(srcDir), this);
  auto descFont = descLabel->font();
  descFont.setPointSize(DESCRIPTION_POINTSIZE);
  descLabel->setFont(descFont);
  descLabel->setContentsMargins(0, 0, HEADER_MARGIN, HEADER_MARGIN * 2);
  headeLeftDescriptionLayout->addWidget(descLabel);
  headeLeftDescriptionLayout->addStretch();
  
  headerLeftLayout->addLayout(headerLeftAuroraLayout);
  headerLeftLayout->addLayout(headeLeftDescriptionLayout);
  //================================================ HEADER Left : Tool Logo-Name-Description--


  //================================================ HEADER Right : Company Logo++
  QHBoxLayout* headerRightLayout = new QHBoxLayout();

  auto logoLabel = new QLabel(this);
  std::filesystem::path labelPath = sourcesPath / ".." / "share" / "aurora" / "etc" / "quickLogic-logo-w-reg-mark.png";
  auto logoPixmap = QIcon(QString::fromStdString(labelPath.string())).pixmap(QSize(355,70));
  if (!logoPixmap.isNull()) logoLabel->setPixmap(logoPixmap);
  headerRightLayout->addWidget(logoLabel, 0, Qt::AlignRight | Qt::AlignTop);
  //================================================ HEADER Right : Company Logo--


  headerLayout->addLayout(headerLeftLayout);
  headerLayout->addLayout(headerRightLayout);


  // Main layout
  auto mainLayout = new QVBoxLayout(this);
  mainLayout->setSpacing(PAGE_SPACING);
  mainLayout->addLayout(headerLayout);
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
