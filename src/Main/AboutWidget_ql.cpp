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
#include "AboutWidget.h"

#include <QDesktopServices>
#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>

namespace FOEDAG {
static const auto ETC_DIR = "etc";
static const auto WELCOME_PAGE_DIR = "Welcome_Page";
static const auto DESCRIPTION_FILENAME = "WelcomeDescription.txt";

AboutWidget::AboutWidget(const ProjectInfo &info,
                         const std::filesystem::path &srcPath, QWidget *parent)
    : QDialog{parent} {
  QLabel *label = new QLabel(this);
  QPushButton *close = new QPushButton("Close", this);

  QString text = QString(
                     "<p><b>%1 %2</b></p>"
                     "<p><b>%3</b></p>"
                     "<p>Build on %4</p>"
                     "<p>Build type: %5</p>")
                     .arg(info.name, info.version, getTagLine(srcPath),
                          __DATE__, info.build_type);
  if (!info.url.isEmpty()) {
    text += QString(
                "<p>From revision <a "
                "href=\"%1%2\">%2</a></p>")
                .arg(info.url, info.git_hash);
  }
  if (info.showLicense) {
    text += QString("<p>%1</p>").arg(License());
  }
  label->setText(text);
  label->setAlignment(Qt::AlignTop);
  connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
    QDesktopServices::openUrl(QUrl(link));
  });
  label->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                 Qt::LinksAccessibleByMouse);
  connect(close, &QPushButton::clicked, this, &AboutWidget::close);
  setLayout(new QGridLayout);
  QHBoxLayout *closeBtnLayout = new QHBoxLayout;
  closeBtnLayout->addStretch();
  closeBtnLayout->addWidget(close);
  layout()->addWidget(label);
  layout()->addItem(closeBtnLayout);
  setWindowTitle(QString("About %1").arg(info.name));

  setFixedSize(sizeHint() + QSize(10, 10));
}

QString AboutWidget::License() {
  const QString license = R"(

<p>Copyright © 2022 Quicklogic Corp</p>

<p>Aurora™ is based on FOEDAG (https://github.com/QuickLogic-Corp/FOEDAG)</p>

<p>FOEDAG original source: (https://github.com/os-fpga/FOEDAG)</p>

<p>Copyright 2022 The Foedag team</p>

<p>FOEDAG GPL License</p>

<p>Copyright (c) 2022 The Open-Source FPGA Foundation</p>

<p>This program is free software: you can redistribute it and/or modify<br>
it under the terms of the GNU General Public License as published by<br>
the Free Software Foundation, either version 3 of the License, or<br>
(at your option) any later version.</p>

<p>This program is distributed in the hope that it will be useful,<br>
but WITHOUT ANY WARRANTY; without even the implied warranty of<br>
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br>
GNU General Public License for more details.</p>

<p>You should have received a copy of the GNU General Public License<br>
along with this program.  If not, see <a>http://www.gnu.org/licenses/</a>.</p>
)";
  return license;
}

QString AboutWidget::getTagLine(const std::filesystem::path &srcDir) {
  std::filesystem::path sourceDir = srcDir / ETC_DIR / WELCOME_PAGE_DIR;
  auto result = QString{};

  std::filesystem::path welcomeDescPath = sourceDir / DESCRIPTION_FILENAME;
  auto descFile = QFile(QString::fromStdString(welcomeDescPath.string()));
  if (!descFile.open(QIODevice::ReadOnly)) return result;

  result = descFile.readAll();
  descFile.close();

  return result.trimmed();
}

}  // namespace FOEDAG
