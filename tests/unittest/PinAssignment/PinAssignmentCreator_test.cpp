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
#include "Compiler/Constraints.h"
#include "Main/ToolContext.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "PinAssignment/PackagePinsView.h"
#include "PinAssignment/PinsBaseModel.h"
#include "TestLoader.h"
#include "TestPortsLoader.h"
#include "gtest/gtest.h"

using namespace FOEDAG;

class PinAssignmentCreatorFixture : public testing::Test {
 public:
  PinAssignmentCreatorFixture()
      : compiler(new Compiler),
        context(new ToolContext("", "", "")),
        projectManager(new ProjectManager) {
    compiler->SetConstraints(new Constraints{compiler});
    data.context = context;
    data.target = "testDevice";
    data.commands = getCommands();
  }
  ~PinAssignmentCreatorFixture() {
    delete compiler;
    delete context;
    delete projectManager;
  }

  QStringList getCommands() const {
    QStringList commands;
    for (const auto &c : compiler->getConstraints()->getConstraints())
      commands.append(QString::fromStdString(c));
    return commands;
  }

 protected:
  void TearDown() override { compiler->getConstraints()->reset(); }

 protected:
  Compiler *compiler;
  ToolContext *context;
  ProjectManager *projectManager;
  PinAssignmentData data;
};

TEST_F(PinAssignmentCreatorFixture, GenerateSdc) {
  PinAssignmentCreator creator{data};
  auto model = creator.baseModel();
  model->update("port1", "pin1");
  model->update("port2", "pin2");
  auto actualSdc = creator.generateSdc();
  QString expected{"set_pin_loc port1 pin1\nset_pin_loc port2 pin2\n"};
  EXPECT_EQ(actualSdc, expected);
}

TEST_F(PinAssignmentCreatorFixture, GenerateSdcWithModes) {
  PinAssignmentCreator creator{data};
  auto model = creator.baseModel();
  model->packagePinModel()->updateMode("pin1", "Mode1");
  model->packagePinModel()->updateMode("pin2", "Mode2");
  auto actualSdc = creator.generateSdc();
  QString expected{"set_mode Mode1 pin1\nset_mode Mode2 pin2\n"};
  EXPECT_EQ(actualSdc, expected);
}

TEST_F(PinAssignmentCreatorFixture, ParseConstraintSetPinLoc) {
  compiler->getConstraints()->addConstraint("set_pin_loc a pin1");
  compiler->getConstraints()->addConstraint("set_pin_loc b pin2");
  data.commands = getCommands();

  PinAssignmentCreator::RegisterPackagePinLoader(data.target, new TestLoader{});
  PinAssignmentCreator::RegisterPortsLoader(data.target, new TestPortsLoader{});
  PinAssignmentCreator creator{data};

  auto actualSdc = creator.generateSdc();
  QString expected{"set_pin_loc a pin1\nset_pin_loc b pin2\n"};
  EXPECT_EQ(actualSdc, expected);
}

TEST_F(PinAssignmentCreatorFixture, PortsModelItemChange) {
  compiler->getConstraints()->addConstraint("set_pin_loc a pin1");
  compiler->getConstraints()->addConstraint("set_pin_loc b pin2");
  data.commands = getCommands();

  PinAssignmentCreator::RegisterPackagePinLoader(data.target, new TestLoader{});
  PinAssignmentCreator::RegisterPortsLoader(data.target, new TestPortsLoader{});
  PinAssignmentCreator creator{data};

  auto ppView = creator.GetPackagePinsWidget()->findChild<PackagePinsView *>();
  if (ppView) ppView->SetPort("pin2", "c");

  auto actualSdc = creator.generateSdc();
  std::string expected{"set_pin_loc a pin1\nset_pin_loc c pin2\n"};
  EXPECT_EQ(actualSdc.toStdString(), expected);
}

TEST_F(PinAssignmentCreatorFixture, ParseConstraintSetPinLocBus) {
  compiler->getConstraints()->addConstraint("set_pin_loc d@0% pin1");
  compiler->getConstraints()->addConstraint("set_pin_loc d@1% pin2");
  data.commands = getCommands();

  PinAssignmentCreator::RegisterPackagePinLoader(data.target, new TestLoader{});
  PinAssignmentCreator::RegisterPortsLoader(data.target, new TestPortsLoader{});
  PinAssignmentCreator creator{data};

  auto actualSdc = creator.generateSdc();
  QString expected{"set_pin_loc d[0] pin1\nset_pin_loc d[1] pin2\n"};
  EXPECT_EQ(actualSdc, expected);
}

TEST_F(PinAssignmentCreatorFixture, ClearModeSelection) {
  compiler->getConstraints()->addConstraint("set_pin_loc a pin1");
  compiler->getConstraints()->addConstraint("set_mode Mode1Tx pin1");
  data.commands = getCommands();

  PinAssignmentCreator::RegisterPackagePinLoader(data.target, new TestLoader{});
  PinAssignmentCreator::RegisterPortsLoader(data.target, new TestPortsLoader{});
  PinAssignmentCreator creator{data};

  auto ppView = creator.GetPackagePinsWidget()->findChild<PackagePinsView *>();
  if (ppView) ppView->SetPort("pin1", QString{});

  auto actualSdc = creator.generateSdc();
  std::string expected{};  // should be empty
  EXPECT_EQ(actualSdc.toStdString(), expected);
}

TEST_F(PinAssignmentCreatorFixture, ParseConstraintSetMode) {
  compiler->getConstraints()->addConstraint("set_pin_loc a pin1");
  compiler->getConstraints()->addConstraint("set_pin_loc b pin2");
  compiler->getConstraints()->addConstraint("set_pin_loc c pin3");
  compiler->getConstraints()->addConstraint("set_mode Mode1Tx pin1");
  compiler->getConstraints()->addConstraint("set_mode Mode2Rx pin2");
  compiler->getConstraints()->addConstraint("set_mode Mode2Rx pin3");
  data.commands = getCommands();

  PinAssignmentCreator::RegisterPackagePinLoader(data.target, new TestLoader{});
  PinAssignmentCreator::RegisterPortsLoader(data.target, new TestPortsLoader{});
  PinAssignmentCreator creator{data};

  auto actualSdc = creator.generateSdc();
  std::string expected{
      "set_pin_loc a pin1\nset_pin_loc b pin2\nset_pin_loc c pin3\nset_mode "
      "Mode1Tx pin1\nset_mode Mode2Rx pin2\nset_mode Mode2Rx pin3\n"};
  EXPECT_EQ(actualSdc.toStdString(), expected);
}

TEST_F(PinAssignmentCreatorFixture, GetPackagePinsWidget) {
  PinAssignmentCreator creator{data};
  EXPECT_NE(creator.GetPackagePinsWidget(), nullptr);
}

TEST_F(PinAssignmentCreatorFixture, GetPortsWidget) {
  PinAssignmentCreator creator{data};
  EXPECT_NE(creator.GetPortsWidget(), nullptr);
}

TEST_F(PinAssignmentCreatorFixture, SearchPortsFileEmptyFile) {
  auto file = PinAssignmentCreator::searchPortsFile(
      QString::fromStdString(std::filesystem::current_path().string()));
  EXPECT_EQ(file, QString());
}
