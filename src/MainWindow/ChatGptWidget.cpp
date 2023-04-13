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
#include "ChatGptWidget.h"

#include <QApplication>
#include <QPainter>

namespace FOEDAG {

QSize ListViewDelegate::iconSize = QSize(60, 60);
int ListViewDelegate::padding = 5;

ListViewDelegate::ListViewDelegate() {}

ListViewDelegate::~ListViewDelegate() {}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  if (!index.isValid()) return QSize();

  QString headerText = index.data(HeaderRole).toString();
  QString subheaderText = index.data(SubheaderRole).toString();

  QFont headerFont = QApplication::font();
  headerFont.setBold(true);
  QFont subheaderFont = QApplication::font();
  QFontMetrics headerFm(headerFont);
  QFontMetrics subheaderFm(subheaderFont);

  auto widgetSize{option.widget->size() - QSize{padding, 0}};
  /* No need for x,y here. we only need to calculate the height given the width.
   * Note that the given height is 0. That is because boundingRect() will return
   * the suitable height if the given geometry does not fit. And this is exactly
   * what we want.
   */
  QRect headerRect = headerFm.boundingRect(
      0, 0, widgetSize.width() - iconSize.width(), option.rect.height(),
      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, headerText);
  QRect subheaderRect = subheaderFm.boundingRect(
      0, 0, widgetSize.width() - iconSize.width(), option.rect.height(),
      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, subheaderText);

  QSize size(widgetSize.width(),
             headerRect.height() + subheaderRect.height() + 3 * padding);

  /* Keep the minimum height needed in mind. */
  if (size.height() < iconSize.height()) size.setHeight(iconSize.height());

  return size;
}

void ListViewDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
  if (!index.isValid()) return;

  painter->save();
  QStyleOptionViewItem opt = option;
  initStyleOption(&opt, index);

  if (opt.state & QStyle::State_Selected)
    painter->fillRect(opt.rect, opt.palette.highlight());

  else if (index.row() % 2 == 1) {
    painter->fillRect(opt.rect, opt.palette.alternateBase());
  }

  QString headerText = index.data(HeaderRole).toString();
  QString subheaderText = index.data(SubheaderRole).toString();

  QFont headerFont = QApplication::font();
  headerFont.setBold(true);
  QFont subheaderFont = QApplication::font();
  QFontMetrics headerFm(headerFont);
  QFontMetrics subheaderFm(subheaderFont);

  auto widgetSize{opt.widget->size() - QSize{padding, 0}};
  /*
   * The x,y coords are not (0,0) but values given by 'option'. So, calculate
   * the rects again given the x,y,w. Note that the given height is 0. That is
   * because boundingRect() will return the suitable height if the given
   * geometry does not fit. And this is exactly what we want.
   */
  QRect headerRect = headerFm.boundingRect(
      opt.rect.left() + iconSize.width(), opt.rect.top() + padding,
      widgetSize.width() - iconSize.width(), 0,
      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, headerText);
  QRect subheaderRect = subheaderFm.boundingRect(
      headerRect.left(), headerRect.bottom() + padding,
      widgetSize.width() - iconSize.width(), 0,
      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, subheaderText);

  painter->setPen(Qt::black);

  painter->setFont(headerFont);
  painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                    headerText);

  painter->setFont(subheaderFont);
  painter->drawText(subheaderRect,
                    Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                    subheaderText);

  QRect iconRect{opt.rect.topLeft() + QPoint{padding, padding},
                 QSize{iconSize.width(), iconSize.width()} -
                     QSize{padding * 2, padding * 2}};
  if (index.row() % 2 == 1)
    painter->drawImage(iconRect, QImage{":/images/chat-bubble-check.png"});
  else
    painter->drawImage(iconRect, QImage{":/images/profile-circle.png"});

  painter->restore();
}

}  // namespace FOEDAG
