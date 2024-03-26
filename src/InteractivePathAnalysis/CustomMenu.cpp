/**
  * @file CustomMenu.cpp
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

#include "CustomMenu.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace FOEDAG {

CustomMenu::CustomMenu(QPushButton* caller) : QWidget(caller) {
  setWindowFlags(Qt::Popup);

  QVBoxLayout* layout = new QVBoxLayout;
  QWidget::setLayout(layout);

  m_contentLayout = new QVBoxLayout;
  m_contentLayout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(m_contentLayout);

  QHBoxLayout* buttonsLayout = new QHBoxLayout;
  layout->addLayout(buttonsLayout);

  m_bnCancel = new QPushButton(tr("Cancel"));
  m_bnDone = new QPushButton(tr("Done"));

  connect(m_bnCancel, &QPushButton::clicked, this, [this]() { hide(); });
  connect(m_bnDone, &QPushButton::clicked, this, [this]() {
    m_isAccepted = true;
    hide();
    emit accepted();
  });

  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(m_bnCancel);
  buttonsLayout->addWidget(m_bnDone);
  buttonsLayout->addStretch(1);

  connect(caller, &QPushButton::clicked, this, [this]() {
    if (!isVisible()) {
      QWidget* parentWidget = qobject_cast<QWidget*>(parent());
      QPoint newPos = parentWidget->mapToGlobal(
          QPoint(0, 0));  // global coord system because of popup property
      newPos.setY(newPos.y() + parentWidget->height());
      popup(newPos);
    }
  });

  hide();  // initially hide
}

void CustomMenu::setButtonToolTips(const QString& toolTipForDoneButton,
                                   const QString& toolTipForCancelButton) {
  m_bnDone->setToolTip(toolTipForDoneButton);
  m_bnCancel->setToolTip(toolTipForCancelButton);
}

void CustomMenu::addContentLayout(QLayout* layout) {
  m_contentLayout->addLayout(layout);
}

void CustomMenu::popup(QPoint pos) {
  m_isAccepted = false;
  QWidget::show();  // show first before move, otherwise on first run we will
                    // have not proper widget size due to not initilized
                    // geometry
  switch (m_alignment) {
    case Qt::AlignLeft: /*do nothing*/
      break;
    case Qt::AlignRight:
      pos.setX(pos.x() - width());
      break;
    case Qt::AlignCenter:
      pos.setX(pos.x() - 0.5 * width());
      break;
  }
  move(pos);
}

void CustomMenu::mousePressEvent(QMouseEvent* event) {
  if (!rect().contains(event->pos())) {
    hide();
  }
  QWidget::mousePressEvent(event);
}

void CustomMenu::hideEvent(QHideEvent* event) {
  if (!m_isAccepted) {
    emit declined();
  }
  QWidget::hideEvent(event);
}

}  // namespace FOEDAG