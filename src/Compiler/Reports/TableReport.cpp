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

#include "TableReport.h"

namespace FOEDAG {

TableReport::TableReport(const ColumnValues &columns,
                         const TableData &linesData, const QString &name,
                         const TableMetaData &metaData)
    : m_columns{columns},
      m_linesData{linesData},
      m_tableMetaData{metaData},
      m_name{name} {}

const IDataReport::ColumnValues &TableReport::getColumns() const {
  return m_columns;
}

const IDataReport::TableData &TableReport::getData() const {
  return m_linesData;
}

const QString &TableReport::getName() const { return m_name; }

bool TableReport::isEmpty() const {
  return m_linesData.empty() || m_columns.empty();
}

const IDataReport::TableMetaData &TableReport::getMetaData() const {
  return m_tableMetaData;
}

}  // namespace FOEDAG
