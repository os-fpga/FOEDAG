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

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <filesystem>

#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

class QTreeWidget;
class QTreeWidgetItem;

namespace FOEDAG {

struct Module {
  QString name;
  QString instName;
  QString file;
  QString line;
  QString instFile;
  QString instLine;
  QVector<Module *> moduleInst;
};

class HierarchyView : public QObject {
  Q_OBJECT

 public:
  HierarchyView(const std::filesystem::path &ports);
  void setPortsFile(const std::filesystem::path &ports);
  void update();
  void clean();

  QTreeWidget *widget();

 signals:
  void openFile(const QString &file, int line);
  void topModuleFile(const QString &);

 private slots:
  void treeWidgetContextMenu(const QPoint &pos);
  void OpenModuleInstance(QTreeWidgetItem *item, int column);

 private:
  QTreeWidgetItem *addItem(QTreeWidgetItem *parent, Module *module);
  void parseJson(json &jsonObject);
  void emitOpenFile(QTreeWidgetItem *item, int column);
  void emitOpenInstFile(QTreeWidgetItem *item, int column);

 private:
  QTreeWidget *m_treeWidget{};
  std::filesystem::path m_portsFile;
  QMap<int, QString> m_files;
  QVector<Module> m_topVector;
};

}  // namespace FOEDAG
