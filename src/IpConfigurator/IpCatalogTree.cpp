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
#include "IpConfigurator/IpCatalogTree.h"

#include <QTreeWidgetItem>

#include "IPGenerate/IPCatalog.h"
#include "IPGenerate/IPCatalogBuilder.h"
#include "MainWindow/Session.h"

extern FOEDAG::Session* GlobalSession;

using namespace FOEDAG;

bool tclCmdExists(const QString& cmdName) {
  bool exists = false;
  int ok = TCL_ERROR;
  // If [info commands cmdName] returns nothing, the command doesn't exist
  QString cmd = QString("expr {[llength [info commands %1]] > 0}").arg(cmdName);
  // result == "0": Command doesn't exist
  // result == "1": Command does exist
  auto result = GlobalSession->TclInterp()->evalCmd(cmd.toStdString(), &ok);

  if (ok == TCL_OK && result != "0") {
    exists = true;
  }

  return exists;
}

IpCatalogTree::IpCatalogTree(QWidget* parent /*nullptr*/)
    : QTreeWidget(parent) {
  this->setHeaderLabel("Available");
  refresh();
  connect(this, &QTreeWidget::itemSelectionChanged, this,
          &IpCatalogTree::itemSelectionHasChanged);
  connect(this, &QTreeWidget::itemDoubleClicked, this,
          &IpCatalogTree::openIpSettings);
}

void IpCatalogTree::refresh() {
  // TODO @skyler-rs AUG-2022 In future updates we plan to allow a user
  // catalog path. This path should be loaded in addition to the default
  std::filesystem::path UserCatalogPath = std::filesystem::path("");
  std::filesystem::path IpCatalogPath =
      GlobalSession->Context()->DataPath() / "IP_Catalog";
  std::vector<std::filesystem::path> IpPaths{IpCatalogPath, UserCatalogPath};

  QStringList ips;
  ips = getAvailableIPs(IpPaths);

  // If available IPs have changed
  if (ips != prevIpCatalogResults) {
    this->clear();
    // Add a tree entry for each IP name
    for (const auto& ip : std::as_const(ips)) {
      QTreeWidgetItem* item = new QTreeWidgetItem();
      item->setText(0, ip);
      this->addTopLevelItem(item);
    }
    sortItems(0, Qt::SortOrder::AscendingOrder);
    prevIpCatalogResults = ips;
  }
}

QStringList IpCatalogTree::getAvailableIPs(
    const std::vector<std::filesystem::path>& paths) {
  QStringList ips;

  // Load IPs
  loadIps(paths);

  // Request loaded IPs
  if (tclCmdExists("ip_catalog")) {
    std::string result = GlobalSession->TclInterp()->evalCmd("ip_catalog");
    ips = QString::fromStdString(result).trimmed().split(" ");
  }

  return ips;
}

void IpCatalogTree::loadIps(const std::vector<std::filesystem::path>& paths) {
  for (const auto& path : paths) {
    if (std::filesystem::exists(path)) {
      GlobalSession->GetCompiler()->BuildLiteXIPCatalog(path.lexically_normal(),
                                                        true);
    }
  }
}

void IpCatalogTree::itemSelectionHasChanged() {
  auto selected = selectedItems();
  if (!selected.isEmpty()) {
    auto ip = selected.first()->text(0);
    auto def =
        GlobalSession->GetCompiler()->GetIPGenerator()->Catalog()->Definition(
            ip.toStdString());
    if (def && !def->Valid()) {
      IPCatalogBuilder builder{GlobalSession->GetCompiler()};
      builder.buildLiteXIPFromGenerator(
          GlobalSession->GetCompiler()->GetIPGenerator()->Catalog(),
          def->FilePath());
    }
  }
  emit ipReady();
}
