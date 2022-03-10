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

#include "ProjNavigator/sources_form.h"
#include "tcltest/TestingUtils.h"

TCL_TEST(projnavigator_show) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->show();
  return TCL_OK;
}

TCL_TEST(projnavigator_close) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->hide();
  return TCL_OK;
}

TCL_TEST(open_project) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->TestOpenProject(argc, argv);
  srcForm->show();
  return TCL_OK;
}

TCL_TEST(create_design) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->TclCreateDesign(argc, argv);
  return TCL_OK;
}

TCL_TEST(add_files) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->TclAddOrCreateFiles(argc, argv);
  return TCL_OK;
}

TCL_TEST(set_active_design) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->TclSetActiveDesign(argc, argv);
  return TCL_OK;
}

TCL_TEST(set_top_module) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->TclSetTopModule(argc, argv);
  return TCL_OK;
}

TCL_TEST(set_as_target) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->TclSetAsTarget(argc, argv);
  return TCL_OK;
}
