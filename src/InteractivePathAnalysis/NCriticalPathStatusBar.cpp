/**
  * @file NCriticalPathStatusBar.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NCriticalPathStatusBar.h"

#include <QHBoxLayout>
#include <QLabel>

#include "NCriticalPathTheme.h"

namespace FOEDAG {

NCriticalPathStatusBar::NCriticalPathStatusBar(QWidget* parent)
    : QWidget(parent) {
  QHBoxLayout* layout = new QHBoxLayout;
  int borderSize = NCriticalPathTheme::instance().borderSize();
  layout->setContentsMargins(borderSize, 0, 0, 0);
  setLayout(layout);

  m_lbConnectionStatus = new QLabel;
  int indicatorSize = NCriticalPathTheme::instance().statusIndicatorSize();
  m_lbConnectionStatus->setFixedSize(indicatorSize, indicatorSize);
  layout->addWidget(m_lbConnectionStatus);

  m_lbMessage = new QLabel;
  layout->addWidget(m_lbMessage);

  onConnectionStatusChanged(false);
}

void NCriticalPathStatusBar::onConnectionStatusChanged(bool isConnected) {
  int indicatorSize = NCriticalPathTheme::instance().statusIndicatorSize();
  const QColor& okColor =
      NCriticalPathTheme::instance().statusIndicatorOkColor();
  const QColor& busyColor =
      NCriticalPathTheme::instance().statusIndicatorBusyColor();
  m_lbConnectionStatus->setStyleSheet(
      QString("border: 1px solid black; border-radius: %1px; background: %2;")
          .arg(indicatorSize / 2)
          .arg(isConnected ? okColor.name() : busyColor.name()));
}

void NCriticalPathStatusBar::onMessageChanged(const QString& msg) {
  setMessage(msg);
}

void NCriticalPathStatusBar::setMessage(const QString& msg) {
  m_lbMessage->setText(msg);
}

void NCriticalPathStatusBar::clear() { m_lbMessage->setText(""); }

}  // namespace FOEDAG