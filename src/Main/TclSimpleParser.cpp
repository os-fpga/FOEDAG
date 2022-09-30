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

namespace FOEDAG {

std::pair<bool, std::string> TclSimpleParser::parse(
    const std::string &tclFile) {
  QFile file{QString::fromStdString(tclFile)};
  if (!file.open(QFile::ReadOnly))
    return std::make_pair(false, "Fail to open file " + tclFile);
  const QString content = file.readAll();

  int counter{0};
  if (content.contains("ipgenerate")) counter++;
  if (content.contains("analyze")) counter++;
  if (content.contains("synth") || content.contains("synthesize")) counter++;
  if (content.contains("packing")) counter++;
  if (content.contains("globp") || content.contains("global_placement"))
    counter++;
  if (content.contains("place")) counter++;
  if (content.contains("route")) counter++;
  if (content.contains("sta")) counter++;
  if (content.contains("power")) counter++;
  if (content.contains("bitstream")) counter++;

  return std::make_pair(true, std::to_string(counter));
}

}  // namespace FOEDAG
