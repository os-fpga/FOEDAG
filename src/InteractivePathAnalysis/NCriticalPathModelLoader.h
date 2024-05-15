/**
  * @file NCriticalPathReportParser.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-05-14
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QThread>
#include <map>
#include <memory>

#include "NCriticalPathReportParser.h"

namespace FOEDAG {

class NCriticalPathItem;

struct ItemsHelperStruct {
  std::vector<std::pair<NCriticalPathItem*, NCriticalPathItem*>> items;
  std::map<QString, int> inputNodes;
  std::map<QString, int> outputNodes;
};
using ItemsHelperStructPtr = std::shared_ptr<ItemsHelperStruct>;

class NCriticalPathModelLoader : public QThread {
  Q_OBJECT
 public:
  explicit NCriticalPathModelLoader(QString&& rawData)
      : QThread(nullptr), m_rawData(rawData) {}
  ~NCriticalPathModelLoader() {}

 signals:
  void itemsReady(const FOEDAG::ItemsHelperStructPtr&);

 protected:
  void run() override final;

 private:
  QString m_rawData;

  void createItems(const std::vector<GroupPtr>& groups,
                   const std::map<int, std::pair<int, int>>& metadata);
  std::tuple<QString, QString, QString> extractRow(QString) const;
};

}  // namespace FOEDAG

Q_DECLARE_METATYPE(FOEDAG::ItemsHelperStructPtr)