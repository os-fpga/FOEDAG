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
#include <QTextEdit>
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
                     "<p><b>%1</b></p>"
                     "<p><b>%2</b></p>"
                     "<p>Version: %3</p>"
                     "<p>Build: %4</p>"
                     "<p>Hash: %5</p>"
                     "<p>Date: %6</p>"
                     "<p>Type: %7</p>")
                     .arg(info.name, getTagLine(srcPath), info.version,
                          info.build, info.git_hash, __DATE__, info.build_type);
  QTextEdit *license{nullptr};
  if (!info.licenseFile.isEmpty()) {
    license = new QTextEdit{this};
    license->setTextInteractionFlags(license->textInteractionFlags() &
                                     ~Qt::TextEditable);
    license->setHtml(License(srcPath, info.licenseFile));
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
  if (license) layout()->addWidget(license);
  layout()->addItem(closeBtnLayout);
  setWindowTitle(QString("About %1").arg(info.name));

  setMinimumWidth(400);
}

QString AboutWidget::License(const std::filesystem::path &srcDir,
                             const QString &file) {
  std::filesystem::path sourceDir = srcDir / ETC_DIR / file.toStdString();
  auto result = QString{};
  auto descFile = QFile(QString::fromStdString(sourceDir.string()));
  if (!descFile.open(QIODevice::ReadOnly)) return result;

  result = descFile.readAll();
  descFile.close();

  // convert plain text to html
  QTextEdit textEdit;
  textEdit.setPlainText(result.trimmed());
  result = textEdit.toHtml();

  return result.trimmed();
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
