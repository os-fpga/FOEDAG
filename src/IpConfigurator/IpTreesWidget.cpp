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
#include "IpConfigurator/IpTreesWidget.h"

#include <QVBoxLayout>

using namespace FOEDAG;

Q_GLOBAL_STATIC(IpTreesWidget, iptrees)

IpTreesWidget* IpTreesWidget::Instance() { return iptrees(); }

void IpTreesWidget::Init() {
  this->setObjectName("IpTreesWidget");

  // Using m_splitter as an indicator if this widget has been init'd yet
  if (!m_splitter) {
    // Main VLayout
    QVBoxLayout* vLayout = new QVBoxLayout();
    this->setLayout(vLayout);
    vLayout->setContentsMargins(0, 0, 0, 0);

    // Vertical Splitter for Trees
    m_splitter = new QSplitter();
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->setObjectName("IpTreesVSplitter");
    vLayout->addWidget(m_splitter);

    // Ip Catalog Tree
    m_catalog_tree = new IpCatalogTree(this);
    m_splitter->addWidget(m_catalog_tree);

    // // Ip Instance Tree
    // m_instances_tree = new IpInstancesTree(this);
    // m_splitter->addWidget(m_instances_tree);
  }
}
