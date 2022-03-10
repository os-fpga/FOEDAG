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

#include "NewFile/new_file.h"
#include "tcltest/TestingUtils.h"

TCL_TEST(newfile_show) {
  FOEDAG::NewFile* newFile = (FOEDAG::NewFile*)(clientData);
  newFile->StartNewFile();
  return TCL_OK;
}

TCL_TEST(newfile_close) {
  FOEDAG::NewFile* newFile = (FOEDAG::NewFile*)(clientData);
  newFile->StopNewFile();
  return TCL_OK;
}

TCL_TEST(newfile) {
  if (argc > 1) {
    FOEDAG::NewFile* newFile = (FOEDAG::NewFile*)(clientData);
    newFile->TclNewFile(argv[1]);
  }
  return TCL_OK;
}
