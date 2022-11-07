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
#include "TestLoader.h"

namespace FOEDAG {

TestLoader::TestLoader() : PackagePinsLoader{nullptr} {}

std::pair<bool, QString> TestLoader::load(const QString &fileName) {
  if (!m_model) return std::make_pair(false, "Package pin model is missing");
  QVector<QString> tmp{Voltage2 + 1};
  QStringList oneLine = QStringList::fromVector(tmp);
  PackagePinGroup group{};
  oneLine[PinName] = "pin1";
  group.pinData.append({oneLine});
  oneLine[PinName] = "pin2";
  group.pinData.append({oneLine});
  oneLine[PinName] = "pin3";
  group.pinData.append({oneLine});
  m_model->append(group);
  m_model->initListModel();

  // fill header data
  HeaderData header{};
  const int headerCount{16};
  for (int i = 0; i < headerCount; i++) {
    m_model->appendHeaderData(header);
  }
  return std::make_pair(true, QString{});
}

std::pair<bool, QString> TestLoader::loadHeader(const QString &fileName) {
  QStringList modesRx{{"Mode1Rx", "Mode2Rx"}};
  QStringList modesTx{{"Mode1Tx", "Mode2Tx"}};

  if (!modesRx.isEmpty()) modesRx.push_front({});  // one empty element
  if (!modesTx.isEmpty()) modesTx.push_front({});  // one empty element

  m_model->modeModelTx()->setStringList(modesTx);
  m_model->modeModelRx()->setStringList(modesRx);
  return std::make_pair(true, QString{});
}

}  // namespace FOEDAG
