/**
  * @file NCriticalPathModel.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
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

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTimer>
#include <QVariant>
#include <map>

#include "NCriticalPathReportParser.h"
#include "NCriticalPathModelLoader.h"

namespace FOEDAG {

class NCriticalPathItem;

class NCriticalPathModel final : public QAbstractItemModel {
  Q_OBJECT

  const int LINE_LIMITER_FILTER_TIME_MS = 1000;
  const int LINE_CHAR_NUM_MIN = 10;

 public:
  explicit NCriticalPathModel(QObject* parent = nullptr);
  ~NCriticalPathModel() override final;

  const std::map<QString, int>& inputNodes() const { return m_inputNodes; }
  const std::map<QString, int>& outputNodes() const { return m_outputNodes; }

  void clear();

  QVariant data(const QModelIndex& index, int role) const override final;
  Qt::ItemFlags flags(const QModelIndex& index) const override final;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override final;
  QModelIndex index(
      int row, int column,
      const QModelIndex& parent = QModelIndex()) const override final;
  QModelIndex parent(const QModelIndex& index) const override final;
  int rowCount(const QModelIndex& parent = QModelIndex()) const override final;
  int columnCount(
      const QModelIndex& parent = QModelIndex()) const override final;

  bool isSelectable(const QModelIndex& index) const;

 public slots:
  void loadFromString(QString rawData);
  void loadItems(const ItemsHelperStructPtr& itemsPtr);
  void limitLineCharsNum(std::size_t lineCharsMaxNum);

 signals:
  void loadFinished();
  void cleared();

 private:
  NCriticalPathItem* m_rootItem = nullptr;

  std::map<QString, int> m_inputNodes;
  std::map<QString, int> m_outputNodes;

  QTimer m_lineLimiterTimer;  // a single-shot timer is used to unite multiple
                              // requests into one
  std::size_t m_lineCharsMaxNum = 0;

  void insertNewItem(NCriticalPathItem* parentItem, NCriticalPathItem* newItem);
  int findRow(NCriticalPathItem*) const;
  int findColumn(NCriticalPathItem*) const;

  void applyLineCharsNum();
};

}  // namespace FOEDAG
