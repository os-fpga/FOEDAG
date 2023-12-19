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

extern const char* foedag_build_date;
namespace FOEDAG {
static const auto ETC_DIR = "etc";
static const auto WELCOME_PAGE_DIR = "Welcome_Page";
static const auto DESCRIPTION_FILENAME = "WelcomeDescription.txt";

AboutWidget::AboutWidget(const ProjectInfo &info,
                         const std::filesystem::path &srcPath, QWidget *parent)
    : QDialog{parent} {
  QLabel *label = new QLabel(this);
  QPushButton *close = new QPushButton("Close", this);

  QHBoxLayout* headerLeftAuroraLayout = new QHBoxLayout();

  QLabel* headerIconLabel = new QLabel(this);
  std::filesystem::path headerIconPath = srcPath / ".." / "share" / "aurora" / ETC_DIR / "aurora_logo.svg";
  auto headerIconPixmap = QIcon(QString::fromStdString(headerIconPath.string())).pixmap(QSize(21,21));
  if (!headerIconPixmap.isNull()) headerIconLabel->setPixmap(headerIconPixmap);
  headerIconLabel->setContentsMargins(0, 0, 5, 0);
  headerLeftAuroraLayout->addWidget(headerIconLabel);
  
  auto headerLabel = new QLabel(this);
  std::filesystem::path headerPath = srcPath / ".." / "share" / "aurora" / ETC_DIR / "aurora_title.svg";
  auto headerPixmap = QIcon(QString::fromStdString(headerPath.string())).pixmap(QSize(131,20));
  if (!headerPixmap.isNull()) headerLabel->setPixmap(headerPixmap);
  headerLeftAuroraLayout->addWidget(headerLabel);
  headerLeftAuroraLayout->addStretch();

  QString text = QString(
                     "<p><b>%1</b></p>"
                     "<p><b>%2</b></p>"
                     "<p>Date     : %3</p>"
                     "<p>Build    : %4</p>")
                     .arg(info.version, getTagLine(srcPath),
                          QString::fromUtf8(foedag_build_date).replace("_"," "), info.build_type);
  if (!info.url.isEmpty()) {
    text += QString(
                "<p>Revision : <a "
                "href=\"%1%2\">%2</a></p>")
                .arg(info.url, info.git_hash);
  }
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
  layout()->addItem(headerLeftAuroraLayout);
  layout()->addWidget(label);
  if (license) layout()->addWidget(license);
  layout()->addItem(closeBtnLayout);
  setWindowTitle(QString("About %1").arg(info.name));

  if(parent != NULL) {
    resize((1*parent->size().width()/2),(5*parent->size().height()/6));
  }
  else {
    setMinimumWidth(500);
  }
}

QString AboutWidget::License(const std::filesystem::path &srcDir,
                             const QString &file) {
  std::filesystem::path sourceDir = srcDir / ".." / "share" / "aurora" / ETC_DIR / file.toStdString();
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
  std::filesystem::path sourceDir = srcDir / ".." / "share" / "aurora" / ETC_DIR / WELCOME_PAGE_DIR;
  auto result = QString{};

  std::filesystem::path welcomeDescPath = sourceDir / DESCRIPTION_FILENAME;
  auto descFile = QFile(QString::fromStdString(welcomeDescPath.string()));
  if (!descFile.open(QIODevice::ReadOnly)) return result;

  result = descFile.readAll();
  descFile.close();

  return result.trimmed();
}

}  // namespace FOEDAG
