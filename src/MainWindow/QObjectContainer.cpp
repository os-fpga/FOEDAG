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
#include "QObjectContainer.h"

#include <QAction>
#include <QWidget>

namespace FOEDAG {

QObjectContainer::QObjectContainer() {}

void QObjectContainer::addObject(QObject *o) {
  if (o) m_objects.append(o);
}

void QObjectContainer::addObjects(const QVector<QObject *> &o) {
  m_objects = o;
}

void QObjectContainer::setEnabled(bool enable) {
  for (auto w : std::as_const(m_objects)) {
    if (auto widget = qobject_cast<QWidget *>(w); widget)
      widget->setEnabled(enable);
    else if (auto action = qobject_cast<QAction *>(w); action)
      action->setEnabled(enable);
  }
}

}  // namespace FOEDAG
