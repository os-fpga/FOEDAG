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
#include "HierarchyView.h"

#include <QDebug>
#include <QFile>
#include <QTreeWidget>

static int FileRole = Qt::UserRole + 1;
static int LineRole = Qt::UserRole + 2;

namespace FOEDAG {

HierarchyView::HierarchyView(const std::filesystem::path &ports)
    : m_treeWidget(new QTreeWidget), m_portsFile(ports) {
  m_treeWidget->setHeaderHidden(true);

  connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem *item, int column) {
            emit openFile(item->data(0, FileRole).toString(),
                          item->data(0, LineRole).toInt());
          });

  update();
}

void HierarchyView::setPortsFile(const std::filesystem::path &ports) {
  m_portsFile = ports;
  update();
}

void HierarchyView::update() {
  clean();
  bool fileParsed{false};

  QString jFile = QString::fromStdString(m_portsFile.string());
  QFile jsonFile{jFile};
  if (jsonFile.exists() &&
      jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Read/parse json from file and update the passed jsonObject w/ new vals
    QString jsonStr = jsonFile.readAll();
    json jsonObject;
    try {
      jsonObject.update(json::parse(jsonStr.toStdString()), true);
      parseJson(jsonObject);
      fileParsed = true;
    } catch (std::exception &e) {
      qWarning() << "Failed to parse " << jFile << ". Error: " << e.what();
    }
  }

  if (fileParsed) {
    QTreeWidgetItem *top = addItem(nullptr, &m_top);
    top->setText(0, m_top.name);
    m_treeWidget->addTopLevelItem(top);
    m_treeWidget->expandAll();
  }
}

QTreeWidget *HierarchyView::widget() { return m_treeWidget; }

void HierarchyView::clean() {
  m_files.clear();
  m_treeWidget->clear();
  qDeleteAll(m_top.moduleInst);
  m_top = {};
}

QTreeWidgetItem *HierarchyView::addItem(QTreeWidgetItem *parent,
                                        Module *module) {
  auto title = QString{"%1 : %2"}.arg(module->instName, module->name);
  QTreeWidgetItem *it = new QTreeWidgetItem{{title}};
  it->setToolTip(0, title);
  it->setData(0, FileRole, module->file);
  it->setData(0, LineRole, module->line);
  if (parent) parent->addChild(it);

  for (auto sub : qAsConst(module->moduleInst)) {
    addItem(it, sub);
  }
  return it;
}

void HierarchyView::parseJson(json &jsonObject) {
  auto files = jsonObject.at("fileIDs");
  for (auto it = files.begin(); it != files.end(); it++) {
    m_files.insert(QString::fromStdString(it.key()).toInt(),
                   QString::fromStdString(it.value().get<std::string>()));
  }

  // top module parsing
  auto hierTree = jsonObject.at("hierTree")[0];
  auto topModule = hierTree.at("topModule");
  m_top.name = QString::fromStdString(topModule.get<std::string>());
  m_top.line = QString::number(hierTree.at("line").get<int>());
  m_top.file = m_files.value(
      QString::fromStdString(hierTree.at("file").get<std::string>()).toInt());
  if (hierTree.contains("moduleInsts")) {
    auto moduleInst = hierTree.at("moduleInsts");
    for (auto it = moduleInst.begin(); it != moduleInst.end(); it++) {
      Module *mod = new Module;
      mod->name = QString::fromStdString(it->at("module").get<std::string>());
      mod->line = QString::number(it->at("line").get<int>());
      mod->file = m_files.value(
          QString::fromStdString(it->at("file").get<std::string>()).toInt());
      mod->instName =
          QString::fromStdString(it->at("instName").get<std::string>());
      m_top.moduleInst.append(mod);
    }
  }

  // all modules parsing
  auto modules = jsonObject.at("modules");
  QVector<Module *> allModules;
  for (auto m = modules.begin(); m != modules.end(); m++) {
    Module *newMod = new Module;
    newMod->name = QString::fromStdString(m.key());
    newMod->line = QString::number(m.value().at("line").get<int>());
    newMod->file = m_files.value(
        QString::fromStdString(m.value().at("file").get<std::string>())
            .toInt());
    allModules.append(newMod);
    if (m.value().contains("moduleInsts")) {
      auto moduleInst = m.value().at("moduleInsts");
      for (auto it = moduleInst.begin(); it != moduleInst.end(); it++) {
        Module *modeInst = new Module;
        modeInst->name = QString::fromStdString(it->at("module"));
        modeInst->line = QString::number(it->at("line").get<int>());
        modeInst->file = m_files.value(
            QString::fromStdString(it->at("file").get<std::string>()).toInt());
        modeInst->instName = QString::fromStdString(it->at("instName"));
        newMod->moduleInst.append(modeInst);
      }
    }
  }

  auto getInst = [allModules](const QString &name) -> Module * {
    for (auto m : allModules)
      if (m->name == name) return m;
    return nullptr;
  };

  for (auto m : qAsConst(allModules)) {
    for (auto inst : qAsConst(m->moduleInst)) {
      auto sub = getInst(inst->name);
      for (auto s : qAsConst(sub->moduleInst)) inst->moduleInst.push_back(s);
    }
  }

  // update top module instances
  for (auto topInst : qAsConst(m_top.moduleInst)) {
    auto inst = getInst(topInst->name);
    for (auto sub : qAsConst(inst->moduleInst)) topInst->moduleInst.append(sub);
  }
}

}  // namespace FOEDAG
