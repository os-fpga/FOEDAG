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
#include "FoedagStyle.h"

#include <QPainter>
#include <QStyleOption>

namespace FOEDAG {

class FoedagStylePrivate {
 public:
  QIcon tabBarcloseButtonIcon{":/img/closetab.png"};
};

FoedagStyle::FoedagStyle(QStyle *style)
    : QProxyStyle(style), d{new FoedagStylePrivate} {}

void FOEDAG::FoedagStyle::drawPrimitive(PrimitiveElement element,
                                        const QStyleOption *opt, QPainter *p,
                                        const QWidget *widget) const {
  switch (element) {
    case PE_IndicatorTabClose: {
      if ((opt->state & State_Enabled) && (opt->state & State_MouseOver))
        proxy()->drawPrimitive(PE_PanelButtonCommand, opt, p, widget);
      QPixmap pixmap = d->tabBarcloseButtonIcon.pixmap(
          QSize(16, 16), QIcon::Normal, QIcon::On);
      proxy()->drawItemPixmap(p, opt->rect, Qt::AlignCenter, pixmap);
      break;
    }
    default:
      QProxyStyle::drawPrimitive(element, opt, p, widget);
      break;
  }
}

}  // namespace FOEDAG
