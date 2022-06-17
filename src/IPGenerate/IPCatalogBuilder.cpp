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

#ifdef _WIN32
#include <Windows.h>
#include <direct.h>
#include <process.h>
#else
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include <QDebug>
#include <QProcess>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <thread>

#include "Compiler/Log.h"
#include "Compiler/ProcessUtils.h"
#include "Compiler/TclInterpreterHandler.h"
#include "Compiler/WorkerThread.h"
#include "IPGenerate/IPCatalogBuilder.h"
#include "MainWindow/Session.h"

extern FOEDAG::Session* GlobalSession;
using namespace FOEDAG;
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

void buildMockUpIPDef(IPCatalog* catalog) {
  std::vector<Connector*> connections;
  Constant* lrange = new Constant(0);
  Parameter* rrange = new Parameter("Width", 0);
  Range range(lrange, rrange);
  Port* port = new Port("clk", Port::Direction::Input, Port::Function::Clock,
                        Port::Polarity::High, range);
  connections.push_back(port);
  IPDefinition* def =
      new IPDefinition("MOCK_IP", "path_to_nowhere", connections);
  catalog->addIP(def);
  // catalog->WriteCatalog(std::cout);
}

bool IPCatalogBuilder::buildLiteXCatalog(
    IPCatalog* catalog, const std::filesystem::path& litexIPgenPath) {
  bool result = true;
  buildMockUpIPDef(catalog);
  return result;
}