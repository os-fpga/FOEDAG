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
#include "CompilerComponent.h"

#include "Compiler/Compiler.h"

namespace FOEDAG {

constexpr auto CompilerMainTag{"Compiler"};
constexpr auto CompilerState{"CompilerState"};

CompilerComponent::CompilerComponent(Compiler *cc) : m_compiler(cc) {}

void CompilerComponent::Save(QXmlStreamWriter *writer) {
  writer->writeStartElement(CompilerMainTag);
  ProjectFileComponent::Save(writer);
  writer->writeAttribute(
      CompilerState,
      QString::number(static_cast<int>(m_compiler->CompilerState())));
  writer->writeEndElement();
}

ErrorCode CompilerComponent::Load(QXmlStreamReader *reader) {
  while (!reader->atEnd()) {
    QXmlStreamReader::TokenType type = reader->readNext();
    if (type == QXmlStreamReader::StartElement &&
        reader->name() == CompilerMainTag) {
      int state = reader->attributes().value(CompilerState).toInt();
      m_compiler->CompilerState(static_cast<Compiler::State>(state));
      break;
    }
  }
  return {};
}

}  // namespace FOEDAG
