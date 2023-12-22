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
#include <filesystem>

class QPushButton;
namespace Ui {
class WelcomePageWidget;
}

namespace FOEDAG {
class WelcomePageWidget final : public QWidget {
  Q_OBJECT
 public:
  WelcomePageWidget(const QString &header,
                    const std::filesystem::path &sourcesPath,
                    QWidget *parent = nullptr);

  // Adds a new QToolButton, representing given action, to the vertical layout
  void addAction(QAction &act);
  void addRecentProject(QAction &act);
  void setConsoleWidget(QWidget *console);

 signals:
  // Emitted whenever current page was closed. Parameter indicates whether it
  // should appear again (when appropriate).
  void welcomePageClosed(bool permatently);

 private:
  void keyPressEvent(QKeyEvent *event) override;
  QPushButton *createActionButton(QAction &action);
  QString getFileContent(const std::filesystem::path &srcDir) const;

  Ui::WelcomePageWidget *ui;
};
}  // namespace FOEDAG

#endif  // WELCOMEPAGEWIDGET_H
