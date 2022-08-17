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
  this->setHeaderLabel("Available IPs");
  refresh();
}

void IpCatalogTree::refresh() {
  QStringList ips = getAvailableIPs("./");

  // If available IPs have changed
  if (ips != prevIpCatalogResults) {
    this->clear();
    // Add a tree entry for each IP name
    for (auto ip : ips) {
      QTreeWidgetItem* item = new QTreeWidgetItem();
      item->setText(0, ip);
      this->addTopLevelItem(item);
    }
  }
}

QStringList IpCatalogTree::getAvailableIPs(QString path) {
  QStringList ips;

  // Load IPs
  loadIps(path);

  // Request loaded IPs
  if (ipsLoaded && tclCmdExists("ip_catalog")) {
    std::string result = GlobalSession->TclInterp()->evalCmd("ip_catalog");
    ips = QString::fromStdString(result).trimmed().split(" ");
  }

  return ips;
}

void IpCatalogTree::loadIps(QString path) {
  if (!ipsLoaded && tclCmdExists("add_litex_ip_catalog")) {
    QString cmd = QString("add_litex_ip_catalog %1").arg(path);
    int ok = TCL_ERROR;
    GlobalSession->TclInterp()->evalCmd(cmd.toStdString(), &ok);
    ipsLoaded = (ok == TCL_OK);
  }
}
