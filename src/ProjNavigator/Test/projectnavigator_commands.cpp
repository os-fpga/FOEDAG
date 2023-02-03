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

#include <QFileInfo>
#include <QTextStream>
#include <iostream>

#include "Main/ProjectFile/ProjectFileLoader.h"
#include "MainWindow/Session.h"
#include "ProjNavigator/sources_form.h"
#include "tcl_command_integration.h"
#include "tclutils/TclUtils.h"

extern FOEDAG::Session* GlobalSession;

TCL_COMMAND(projnavigator_show) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->show();
  return TCL_OK;
}

TCL_COMMAND(projnavigator_close) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  srcForm->hide();
  return TCL_OK;
}

TCL_COMMAND(open_project) {
  FOEDAG::SourcesForm* srcForm = static_cast<FOEDAG::SourcesForm*>(clientData);
  QTextStream out(stdout);
  if (argc < 3 || "--file" != QString(argv[1])) {
    out << "-----------open_project ------------\n";
    out << " \n";
    out << " Description: \n";
    out << " Open a project. Show the source file categories and hierarchies. "
           "\n";
    out << " \n";
    out << " Syntax: \n";
    out << " open_project --file <project.ospr> \n";
    out << " \n";
    out << "--------------------------------------\n";
    return TCL_OK;
  }

  QFileInfo fileInfo;
  QString filename{argv[2]};
  fileInfo.setFile(filename);
  if (fileInfo.exists()) {
    FOEDAG::ProjectFileLoader loader(FOEDAG::Project::Instance());
    loader.registerComponent(
        new FOEDAG::ProjectManagerComponent(srcForm->ProjManager()),
        FOEDAG::ComponentId::TaskManager);
    loader.registerComponent(
        new FOEDAG::TaskManagerComponent(
            new FOEDAG::TaskManager(GlobalSession->GetCompiler())),
        FOEDAG::ComponentId::TaskManager);
    loader.registerComponent(
        new FOEDAG::CompilerComponent(GlobalSession->GetCompiler()),
        FOEDAG::ComponentId::Compiler);
    loader.Load(filename);
    srcForm->InitSourcesForm();
  } else {
    out << " Warning : This file <" << filename << "> is not exist! \n";
  }
  srcForm->show();
  return TCL_OK;
}

TCL_COMMAND(create_fileset) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  auto tclCommand = srcForm->createTclCommandIntegarion();
  tclCommand->TclCreateFileSet(argc, argv, std::cout);
  return TCL_OK;
}

TCL_COMMAND(add_files) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  auto tclCommand = srcForm->createTclCommandIntegarion();
  tclCommand->TclAddOrCreateDesignFiles(argc, argv, std::cout);
  return TCL_OK;
}

TCL_COMMAND(set_active) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  auto tclCommand = srcForm->createTclCommandIntegarion();
  tclCommand->TclSetActive(argc, argv, std::cout);
  return TCL_OK;
}

TCL_COMMAND(set_top_module) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  auto tclCommand = srcForm->createTclCommandIntegarion();
  tclCommand->TclSetTopModule(argc, argv, std::cout);
  return TCL_OK;
}

TCL_COMMAND(set_as_target) {
  FOEDAG::SourcesForm* srcForm = (FOEDAG::SourcesForm*)(clientData);
  auto tclCommand = srcForm->createTclCommandIntegarion();
  tclCommand->TclSetAsTarget(argc, argv, std::cout);
  return TCL_OK;
}
