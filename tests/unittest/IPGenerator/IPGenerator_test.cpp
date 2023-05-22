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

#include "IPGenerate/IPGenerator.h"

#include "Compiler/Compiler.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "ProjNavigator/tcl_command_integration.h"
#include "gtest/gtest.h"

namespace FOEDAG {

TEST(IPGenerate, IpInstanceDupes) {
  // The goal of this test is to ensure that IPInstances are unique by
  // module_name If an instance is added with the same module_name, the old one
  // should be removed

  IPCatalog* ipCat = new IPCatalog();
  Compiler* compiler = new Compiler();
  IPGenerator* ipGen = new IPGenerator(ipCat, compiler);

  std::vector<Connector*> connections;
  std::vector<Value*> parameters;
  Constant* lrange = new Constant(0);
  Parameter* rrange = new Parameter("Width", 0);
  Range range(lrange, rrange);
  Port* port = new Port("clk", Port::Direction::Input, Port::Function::Clock,
                        Port::Polarity::High, range);
  connections.push_back(port);
  IPDefinition* def = new IPDefinition(IPDefinition::IPType::Other, "MOCK_IP",
                                       "MOCK_IP_wrapper", "path_to_nowhere",
                                       connections, parameters);
  IPDefinition* def2 = new IPDefinition(IPDefinition::IPType::Other, "MOCK_IP2",
                                        "MOCK_IP2_wrapper", "path_to_nowhere",
                                        connections, parameters);

  ipCat->addIP(def);

  EXPECT_EQ(ipGen->IPInstances().size(), 0)
      << "Ensure the IPInstances is empty so far";

  std::vector<SParameter> params;
  std::vector<SParameter> params2;

  // Add an ip instance
  IPInstance* instance = new IPInstance("duplicateName", "version_num", def,
                                        params, "module_name", "out_file");
  ipGen->AddIPInstance(instance);
  EXPECT_EQ(ipGen->IPInstances().size(), 1)
      << "Ensure the IPInstances count is now 1";

  // Add exact same IP
  ipGen->AddIPInstance(instance);
  EXPECT_EQ(ipGen->IPInstances().size(), 1)
      << "Ensure the IPInstances count is still 1";

  // Add IP with similar values
  IPInstance* instance2 = new IPInstance("duplicateName", "version_num", def,
                                         params, "module_name", "out_file");
  ipGen->AddIPInstance(instance2);
  EXPECT_EQ(ipGen->IPInstances().size(), 1)
      << "Ensure the IPInstances count is still 1";

  // Add IP with different values, but same module name
  IPInstance* instance3 = new IPInstance("newName", "newVersion", def2, params2,
                                         "module_name", "newPath");
  ipGen->AddIPInstance(instance3);
  EXPECT_EQ(ipGen->IPInstances().size(), 1)
      << "Ensure the IPInstances count is still 1";

  // Add IP same values, but diff module name
  IPInstance* instance4 = new IPInstance("duplicateName", "version_num", def,
                                         params, "NEW_MODULE_NAME", "out_file");
  ipGen->AddIPInstance(instance4);
  EXPECT_EQ(ipGen->IPInstances().size(), 2)
      << "Ensure the IPInstances count is now 2 ";
}

TEST(IPGenerate, CheckAllIPPath) {
  IPCatalog* ipCat = new IPCatalog();
  Compiler* compiler = new Compiler();
  IPGenerator* ipGen = new IPGenerator(ipCat, compiler);
  ProjectManager* pm = new ProjectManager{};
  Project::Instance()->setProjectName("testProject");
  compiler->setGuiTclSync(new TclCommandIntegration{pm, nullptr});

  std::vector<Connector*> connections;
  std::vector<Value*> parameters;
  Constant* lrange = new Constant(0);
  Parameter* rrange = new Parameter("Width", 0);
  Range range(lrange, rrange);
  Port* port = new Port("clk", Port::Direction::Input, Port::Function::Clock,
                        Port::Polarity::High, range);
  connections.push_back(port);
  IPDefinition* def = new IPDefinition(IPDefinition::IPType::Other, "MOCK_IP",
                                       "MOCK_IP_wrapper", "path_to_nowhere",
                                       connections, parameters);

  ipCat->addIP(def);

  EXPECT_EQ(ipGen->IPInstances().size(), 0)
      << "Ensure the IPInstances is empty so far";

  std::vector<SParameter> params;
  std::vector<SParameter> params2;

  // Add an ip instance
  IPInstance* instance = new IPInstance("duplicateName", "version_num", def,
                                        params, "module_name", "out_file");
  ipGen->AddIPInstance(instance);

  EXPECT_EQ(ipGen->GetBuildDir(instance),
            "testProject.IPs/path_to_nowhere/module_name");

  EXPECT_EQ(ipGen->GetSimDir(instance),
            "testProject.IPs/path_to_nowhere/module_name/sim");

  EXPECT_EQ(ipGen->GetSimArtifactsDir(instance),
            "testProject.IPs/simulation/path_to_nowhere/module_name");

  EXPECT_EQ(
      ipGen->GetCachePath(instance),
      "testProject.IPs/path_to_nowhere/module_name/MOCK_IP_module_name.json");

  EXPECT_EQ(ipGen->GetTmpCachePath(instance),
            "testProject.IPs/.tmp/path_to_nowhere/module_name/"
            "MOCK_IP_module_name.json");

  EXPECT_EQ(ipGen->GetTmpPath(), "testProject.IPs/.tmp");

  EXPECT_EQ(ipGen->GetProjectIPsPath(), "testProject.IPs");
}

}  // namespace FOEDAG
