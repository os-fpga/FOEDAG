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

#include <QString>

#include "rapidgpt/ChatWidget.h"
#include "rapidgpt/RapidGpt.h"
#include "tclutils/TclUtils.h"

TCL_COMMAND(rapidgpt_show) {
  auto chat = (FOEDAG::ChatWidget*)(clientData);
  chat->show();
  return TCL_OK;
}

TCL_COMMAND(rapidgpt_close) {
  auto chat = (FOEDAG::ChatWidget*)(clientData);
  chat->close();
  return TCL_OK;
}

TCL_COMMAND(rapidgpt_send) {
  if (argc < 2) return TCL_ERROR;
  auto chat = (FOEDAG::ChatWidget*)(clientData);
  auto rapidGpt = chat->findChild<FOEDAG::RapidGpt*>("rapidgpt");

  if (!rapidGpt) return TCL_ERROR;

  QString message = QString::fromStdString(argv[1]);
  rapidGpt->setShowError(false);
  auto res = rapidGpt->sendRapidGpt(message);

  if (res == false && !rapidGpt->errorString().isEmpty()) return TCL_OK;

  return TCL_ERROR;
}
