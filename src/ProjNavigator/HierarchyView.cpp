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
#include <QFileInfo>
#include <QMenu>
#include <QTreeWidget>

#include "Utils/StringUtils.h"

static int FileRole = Qt::UserRole + 1;
static int LineRole = Qt::UserRole + 2;
static int InstFileRole = Qt::UserRole + 3;
static int InstLineRole = Qt::UserRole + 4;
static int TopItemRole = Qt::UserRole + 5;

namespace FOEDAG {

HierarchyView::HierarchyView(const std::filesystem::path &ports)
    : m_treeWidget(new QTreeWidget), m_portsFile(ports) {
  m_treeWidget->setHeaderHidden(true);

  connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this,
          &HierarchyView::OpenModuleInstance);

  m_treeWidget->setExpandsOnDoubleClick(false);
  m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this,
          &HierarchyView::treeWidgetContextMenu);

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
    for (auto &module : m_topVector) {
      QTreeWidgetItem *top = addItem(nullptr, &module);
      if (top) {
        m_treeWidget->addTopLevelItem(top);
      }
    }
    m_treeWidget->expandAll();
  }
}

QTreeWidget *HierarchyView::widget() { return m_treeWidget; }

void HierarchyView::treeWidgetContextMenu(const QPoint &pos) {
  auto item = m_treeWidget->itemAt(pos);
  if (item) {
    QMenu menu{m_treeWidget};
    QAction *showDef = new QAction{"Go to Definition"};
    connect(showDef, &QAction::triggered, this,
            [this, item]() { emitOpenFile(item, 0); });
    if (!item->data(0, TopItemRole).toBool()) {
      QAction *showInst = new QAction{"Go to Instantiation"};
      connect(showInst, &QAction::triggered, this,
              [this, item]() { OpenModuleInstance(item, 0); });
      menu.addAction(showInst);
    }
    menu.addAction(showDef);
    menu.exec(QCursor::pos());
  }
}

void HierarchyView::OpenModuleInstance(QTreeWidgetItem *item, int column) {
  if (item->data(0, TopItemRole).toBool())
    emitOpenFile(item, column);
  else
    emitOpenInstFile(item, column);
}

void HierarchyView::emitOpenFile(QTreeWidgetItem *item, int column) {
  emit openFile(item->data(column, FileRole).toString(),
                item->data(column, LineRole).toInt());
}

void HierarchyView::emitOpenInstFile(QTreeWidgetItem *item, int column) {
  emit openFile(item->data(column, InstFileRole).toString(),
                item->data(column, InstLineRole).toInt());
}

void HierarchyView::clean() {
  m_files.clear();
  m_treeWidget->clear();
  for (auto &m : m_topVector) qDeleteAll(m.moduleInst);
  m_topVector.clear();
}

QTreeWidgetItem *HierarchyView::addItem(QTreeWidgetItem *parent,
                                        Module *module) {
  const bool topModule = (parent == nullptr);
  const QFileInfo info{module->file};
  auto title = QString{"%1 : %2 (%3)"}.arg(module->instName, module->name,
                                           info.fileName());
  if (topModule) {
    title = QString{"%1 (%2)"}.arg(module->name, info.fileName());
  }
  QTreeWidgetItem *it = new QTreeWidgetItem{{title}};
  it->setToolTip(0, title);
  it->setData(0, InstFileRole, module->instFile);
  it->setData(0, InstLineRole, module->instLine);
  it->setData(0, FileRole, module->file);
  it->setData(0, LineRole, module->line);
  it->setData(0, TopItemRole, topModule);
  if (parent) parent->addChild(it);

  for (auto sub : std::as_const(module->moduleInst)) {
    addItem(it, sub);
  }
  return it;
}

void HierarchyView::parseJson(json &jsonObject) {
  auto files = jsonObject.at("fileIDs");
  m_files.insert(0, QString::fromStdString(std::string("")));
  for (auto it = files.begin(); it != files.end(); it++) {
    const auto &[value, ok] = StringUtils::to_number<int>(it.key());
    if (ok) {
      auto file = std::filesystem::path{it.value().get<std::string>()};
      if (file.is_relative()) {
        file = m_portsFile.parent_path() / file;
        file = file.lexically_normal();
      }
      m_files.insert(value, QString::fromStdString(file.string()));
    }
  }

  auto parseModuleInst = [this](json &moduleInst, Module *module) {
    for (auto it = moduleInst.begin(); it != moduleInst.end(); it++) {
      Module *mod = new Module;
      mod->name = QString::fromStdString(it->at("module").get<std::string>());
      mod->instLine = QString::number(it->at("line").get<int>());
      const auto &[num, ok] =
          StringUtils::to_number<int>(it->at("file").get<std::string>());
      if (ok) mod->instFile = m_files.value(num);
      mod->instName =
          QString::fromStdString(it->at("instName").get<std::string>());
      module->moduleInst.append(mod);
    }
  };

  auto parseModule = [this, parseModuleInst](json &moduleInst, Module *module) {
    module->line = QString::number(moduleInst.at("line").get<int>());
    const auto &[num, ok] =
        StringUtils::to_number<int>(moduleInst.at("file").get<std::string>());
    if (ok) module->file = m_files.value(num);
    if (moduleInst.contains("moduleInsts")) {
      auto moduleInsts = moduleInst.at("moduleInsts");
      parseModuleInst(moduleInsts, module);
    }
  };

  // top module parsing
  QString topModuleFile;
  auto hierTree = jsonObject.at("hierTree");
  for (auto it = hierTree.begin(); it != hierTree.end(); it++) {
    auto topModule = it->at("topModule");
    const auto &[topModuleFileId, ok] =
        StringUtils::to_number<int>(it->at("file"));
    if (ok) topModuleFile = m_files.value(topModuleFileId, {});
    Module m_top;
    m_top.name = QString::fromStdString(topModule.get<std::string>());
    parseModule(*it, &m_top);

    // all modules parsing
    auto modules = jsonObject.at("modules");
    QVector<Module *> allModules;
    for (auto m = modules.begin(); m != modules.end(); m++) {
      Module *newMod = new Module;
      allModules.append(newMod);
      newMod->name = QString::fromStdString(m.key());
      auto in = m.value();
      parseModule(in, newMod);
    }

    auto getInst = [allModules](const QString &name) -> Module * {
      for (auto m : allModules)
        if (m->name == name) return m;
      return nullptr;
    };

    // update all modules instances
    for (auto m : std::as_const(allModules)) {
      for (auto inst : std::as_const(m->moduleInst)) {
        if (auto sub = getInst(inst->name)) {
          for (auto s : std::as_const(sub->moduleInst))
            inst->moduleInst.push_back(s);
        }
      }
    }

    // update top module instances
    for (auto topInst : std::as_const(m_top.moduleInst)) {
      if (auto inst = getInst(topInst->name)) {
        for (auto sub : std::as_const(inst->moduleInst))
          topInst->moduleInst.append(sub);
      }
    }

    // set top module instances file and line
    for (auto topInst : std::as_const(m_top.moduleInst)) {
      if (auto inst = getInst(topInst->name)) {
        topInst->file = inst->file;
        topInst->line = inst->line;
      }
    }

    // set all modules instances file and line
    for (auto m : std::as_const(allModules)) {
      for (auto inst : std::as_const(m->moduleInst)) {
        if (auto sub = getInst(inst->name)) {
          inst->file = sub->file;
          inst->line = sub->line;
        }
      }
    }
    m_topVector.push_back(m_top);
  }
  emit this->topModuleFile(topModuleFile);
}

}  // namespace FOEDAG
