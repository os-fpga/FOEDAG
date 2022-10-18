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

#include "PinAssignment/PinAssignmentCreator.h"

#include "Compiler/Compiler.h"
#include "Main/ToolContext.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "PinAssignment/PinsBaseModel.h"
#include "gtest/gtest.h"
using namespace FOEDAG;

class PinAssignmentCreatorFixture : public testing::Test {
 public:
  PinAssignmentCreatorFixture()
      : compiler(new Compiler),
        context(new ToolContext("", "", "")),
        projectManager(new ProjectManager) {}
  ~PinAssignmentCreatorFixture() {
    delete compiler;
    delete context;
    delete projectManager;
  }

 protected:
  Compiler *compiler;
  ToolContext *context;
  ProjectManager *projectManager;
};

TEST_F(PinAssignmentCreatorFixture, GenerateSdc) {
  PinAssignmentCreator creator{projectManager, context, compiler};
  auto model = creator.baseModel();
  model->update("port1", "pin1");
  model->update("port2", "pin2");
  auto actualSdc = creator.generateSdc();
  QString expected{"set_pin_loc port1 pin1\nset_pin_loc port2 pin2\n"};
  EXPECT_EQ(actualSdc, expected);
}

TEST_F(PinAssignmentCreatorFixture, GetPackagePinsWidget) {
  PinAssignmentCreator creator{projectManager, context, compiler};
  EXPECT_NE(creator.GetPackagePinsWidget(), nullptr);
}

TEST_F(PinAssignmentCreatorFixture, GetPortsWidget) {
  PinAssignmentCreator creator{projectManager, context, compiler};
  EXPECT_NE(creator.GetPortsWidget(), nullptr);
}

TEST_F(PinAssignmentCreatorFixture, SearchPortsFileEmptyFile) {
  auto file = PinAssignmentCreator::searchPortsFile(
      QString::fromStdString(std::filesystem::current_path().string()));
  EXPECT_EQ(file, QString());
}
