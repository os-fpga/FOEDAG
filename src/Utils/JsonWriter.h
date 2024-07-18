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
#pragma once

#include <filesystem>
#include <sstream>

namespace FOEDAG {

class JsonWriterIterator {
  friend class JsonStreamWriter;

 public:
  template <class T>
  void operator=(const T &value) {
    if (m_valid) m_stream << value;
    insertEnding();
  }
  void operator=(const std::string &value);
  void operator=(bool value);
  bool isValid() const;

 private:
  explicit JsonWriterIterator(std::ostringstream &stream);
  void insertEnding();

 private:
  std::ostringstream &m_stream;
  bool m_valid{true};
};

/*!
 * \brief The JsonStreamWriter class implement writing json dirrect to the file.
 * Format of the json: object that has key-value pairs
 */
class JsonStreamWriter {
 public:
  /*!
   * \brief JsonStreamWriter create instance that will write json to \a file
   * \param indent, allows to set intent count for formatting
   */
  explicit JsonStreamWriter(const std::filesystem::path &file, int indent = 3);

  /*!
   * Close file before object deleted
   */
  ~JsonStreamWriter();

  /*!
   * \brief insert \param key - \param value pairs into json. Type of \param
   * value will not be changed. E.g. if value type is integer then integer will
   * be inserted.
   */
  void insert(const std::string &key, const std::string &value);
  void insertString(const std::string &key, const std::string &value);

  /*!
   * \brief close dump all data to a file.
   */
  void close();

  /*!
   * \brief operator [] allow insert value for key \param key. This method will
   * make sure that string will be inserted.
   */
  JsonWriterIterator operator[](const std::string &key);

 private:
  void start();

 private:
  std::ostringstream m_json{};
  std::filesystem::path m_file{};
  int m_indent{};
  bool m_closed{false};
};

}  // namespace FOEDAG
