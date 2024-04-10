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
#include "TclSimpleParser.h"

#include <QFile>
#include <QString>

#include "Compiler/CompilerDefines.h"

namespace FOEDAG {

bool TclSimpleParser::parse(const std::string &tclFile,
                            std::vector<uint> &ids) {
  QFile file{QString::fromStdString(tclFile)};
  if (!file.open(QFile::ReadOnly)) return false;

  while (!file.atEnd()) {
    QByteArray line = file.readLine();
    line = line.simplified();
    if (line.startsWith('#')) continue;
    if (line.contains("ipgenerate")) {
      ids.push_back(IP_GENERATE);
    }
    if (line.contains("analyze")) {
      ids.push_back(ANALYSIS);
    }
    if (line.contains("synth") || line.contains("synthesize")) {
      ids.push_back(SYNTHESIS);
    }
    if (line.contains("packing")) {
      ids.push_back(PACKING);
    }
    if (line.contains("place") || line.contains("detailed_placement")) {
      ids.push_back(PLACEMENT);
    }
    if (line.contains("route")) {
      ids.push_back(ROUTING);
    }
    if (line.contains("sta")) {
      ids.push_back(TIMING_SIGN_OFF);
    }
    if (line.contains("power")) {
      ids.push_back(POWER);
    }
    if (line.contains("bitstream")) {
      ids.push_back(BITSTREAM);
    }
    if (line.contains("simulate")) {
      if (line.contains("gate"))
        ids.push_back(SIMULATE_GATE);
      else if (line.contains("pnr"))
        ids.push_back(SIMULATE_PNR);
      else if (line.contains("rtl"))
        ids.push_back(SIMULATE_RTL);
      else if (line.contains("bitstream"))
        ids.push_back(SIMULATE_BITSTREAM);
    }
  }
  return true;
}

}  // namespace FOEDAG
