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

#ifndef WELCOMEPAGEWIDGET_H
#define WELCOMEPAGEWIDGET_H

#include <QWidget>

class QAction;
class QVBoxLayout;
class QPixmap;
class QPushButton;

namespace FOEDAG {
class WelcomePageWidget final : public QWidget {
 public:
  WelcomePageWidget(const QString &header, const QString &sourcesPath,
                    QWidget *parent = nullptr);

  // Adds a new QToolButton, representing given action, to the vertical layout
  void addAction(QAction &act);

 private:
  QPushButton *createActionButton();

  // Reads WelcomeDescription txt file, located in given path. Returns empty
  // string if the file doesn't exist.
  QString getDescription(const QString &sourcesPath) const;

  QVBoxLayout *m_actionsLayout{nullptr};
};
}  // namespace FOEDAG

#endif  // WELCOMEPAGEWIDGET_H
