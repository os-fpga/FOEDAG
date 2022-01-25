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
#pragma once

#include <QGridLayout>

#include "SearchWidget.h"
#include "TclConsoleWidget.h"

namespace FOEDAG {

QWidget *createConsole(Tcl_Interp *interp,
                       std::unique_ptr<ConsoleInterface> iConsole,
                       StreamBuffer *buffer, QWidget *parent = nullptr) {
  QWidget *w = new QWidget{parent};
  w->setLayout(new QGridLayout);
  TclConsoleWidget *console =
      new TclConsoleWidget{interp, std::move(iConsole), buffer};

  SearchWidget *search = new SearchWidget{console};
  QObject::connect(console, &TclConsoleWidget::searchEnable, search,
                   &SearchWidget::search);

  w->layout()->addWidget(console);
  w->layout()->addWidget(search);
  w->layout()->setSpacing(0);
  w->layout()->setContentsMargins(0, 0, 0, 0);
  w->setGeometry(0, 0, 730, 440);

  return w;
}
}  // namespace FOEDAG
