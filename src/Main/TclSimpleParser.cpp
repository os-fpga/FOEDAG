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

struct Token {
  QString value{};
  bool valid{false};
  explicit operator bool() const { return valid; }
  bool operator==(const Token &other) const {
    return value == other.value && valid == other.valid;
  }
};

class Tokenize {
 public:
  explicit Tokenize(const QString &line) {
    m_data = line.simplified().split(' ', Qt::SkipEmptyParts);
  }
  Token GetNext() {
    Token token{};
    if (!m_data.isEmpty()) {
      token = Token{m_data.first(), true};
      m_data.removeFirst();
    }
    return token;
  }

 private:
  QStringList m_data;
};

bool TclSimpleParser::parse(const std::string &tclFile,
                            std::vector<uint> &ids) {
  QFile file{QString::fromStdString(tclFile)};
  if (!file.open(QFile::ReadOnly)) return false;

  while (!file.atEnd()) {
    auto line = file.readLine();
    Tokenize tokenize{line};
    Token token = tokenize.GetNext();
    if (!token) continue;
    if (token == Token{"#", true}) continue;
    if (token == Token{"ipgenerate", true}) {
      ids.push_back(IP_GENERATE);
    }
    if (token == Token{"analyze", true}) {
      ids.push_back(ANALYSIS);
    }
    if (token == Token{"synth", true} || token == Token{"synthesize", true}) {
      ids.push_back(SYNTHESIS);
    }
    if (token == Token{"packing", true}) {
      ids.push_back(PACKING);
    }
    if (token == Token{"place", true} ||
        token == Token{"detailed_placement", true}) {
      ids.push_back(PLACEMENT);
    }
    if (token == Token{"route", true}) {
      ids.push_back(ROUTING);
    }
    if (token == Token{"sta", true}) {
      ids.push_back(TIMING_SIGN_OFF);
    }
    if (token == Token{"power", true}) {
      ids.push_back(POWER);
    }
    if (token == Token{"bitstream", true}) {
      ids.push_back(BITSTREAM);
    }
    if (token == Token{"simulate", true}) {
      token = tokenize.GetNext();
      while (token) {
        if (token == Token{"gate", true})
          ids.push_back(SIMULATE_GATE);
        else if (token == Token{"pnr", true})
          ids.push_back(SIMULATE_PNR);
        else if (token == Token{"rtl", true})
          ids.push_back(SIMULATE_RTL);
        else if (token == Token{"bitstream_fd", true} ||
                 token == Token{"bitstream_bd", true})
          ids.push_back(SIMULATE_BITSTREAM);
        token = tokenize.GetNext();
      }
    }
  }
  return true;
}

}  // namespace FOEDAG
