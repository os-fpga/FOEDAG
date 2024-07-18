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
#include "Utils/JsonWriter.h"

#include <fstream>
#include <iterator>

#include "Utils/StringUtils.h"

namespace FOEDAG {

const char *ending{",\n"};

JsonStreamWriter::JsonStreamWriter(const std::filesystem::path &file,
                                   int indent)
    : m_file(file), m_indent(indent) {
  start();
}

JsonStreamWriter::~JsonStreamWriter() { close(); }

void JsonStreamWriter::insert(const std::string &key,
                              const std::string &value) {
  std::fill_n(std::ostream_iterator<char>(m_json), m_indent, ' ');
  m_json << "\"" << key << "\": " << value << ending;
}

void JsonStreamWriter::insertString(const std::string &key,
                                    const std::string &value) {
  insert(key, "\"" + value + "\"");
}

void JsonStreamWriter::start() { m_json << "{\n"; }

void JsonStreamWriter::close() {
  if (!m_closed) {
    auto content = m_json.str();
    const std::string endingStr{ending};
    if (!content.empty() && StringUtils::endsWith(content, endingStr))
      content = content.substr(0, content.length() - endingStr.length());
    content += "\n}\n";
    std::ofstream steam{m_file};
    steam << content;
    steam.close();
  }
  m_closed = true;
}

JsonWriterIterator JsonStreamWriter::operator[](const std::string &key) {
  std::fill_n(std::ostream_iterator<char>(m_json), m_indent, ' ');
  m_json << "\"" << key << "\": ";
  return JsonWriterIterator{m_json};
}

void JsonWriterIterator::operator=(const std::string &value) {
  if (m_valid) m_stream << "\"" << value << "\"";
  insertEnding();
}

void JsonWriterIterator::operator=(bool value) {
  if (m_valid) m_stream << std::boolalpha << value;
  insertEnding();
}

bool JsonWriterIterator::isValid() const { return m_valid; }

JsonWriterIterator::JsonWriterIterator(std::ostringstream &stream)
    : m_stream(stream) {}

void JsonWriterIterator::insertEnding() {
  m_stream << ending;
  m_valid = false;
}

}  // namespace FOEDAG
