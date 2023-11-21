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

#include <QBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "IPGenerate/IPCatalog.h"

namespace FOEDAG {

class IpConfigWidget : public QWidget {
  Q_OBJECT

 public:
  explicit IpConfigWidget(QWidget* parent = nullptr,
                          const QString& requestedIpName = QString{},
                          const QString& moduleName = QString{});

 private:
  void generateDetailedInformation();
  void Generate(const QString& outputPath = {});
  void CreateParamFields();
  void updateMetaLabel(const IPDetails& details);
  static std::vector<FOEDAG::IPDefinition*> getDefinitions();

  QGroupBox* m_paramsBox{nullptr};
  QLabel m_metaLabel;
  QString m_moduleName{};

  const std::filesystem::path m_baseDirDefault;
  const QString m_requestedIpName;
  VLNV m_meta;
  IPDetails m_details;
};

}  // namespace FOEDAG
