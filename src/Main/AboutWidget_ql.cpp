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
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>

namespace FOEDAG {

AboutWidget::AboutWidget(const ProjectInfo &info, QWidget *parent)
    : QDialog{parent} {
  QLabel *label = new QLabel(this);
  QPushButton *close = new QPushButton("Close", this);
  label->setText(QString("<p><b>%1 %2</b></p>"
                         "<p>Date: %3</p>"
                         "<p>Revision: <a "
                         "href=\"%4%5\">%5</a></p>"
                         "<p>Build: %6</p>"
                         "<p>%7</p>")
                     .arg(info.name, info.version, __DATE__, info.url,
                          info.git_hash, info.build_type, License()));
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

}  // namespace FOEDAG
