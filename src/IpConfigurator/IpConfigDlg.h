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
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>

#include "IPGenerate/IPCatalog.h"

namespace FOEDAG {

class IpConfigDlg : public QDialog {
  Q_OBJECT

 public:
  explicit IpConfigDlg(QWidget* parent = nullptr, QString requestedIpName = "");

 public slots:
  void updateOutputPath();

 private:
  void AddDialogControls(QBoxLayout* layout);
  void CreateParamFields();
  void CreateOutputFields();
  void updateMetaLabel(VLNV info);
  std::vector<FOEDAG::IPDefinition*> getDefinitions();

  QGroupBox paramsBox{"Parameters", this};
  QGroupBox outputBox{"Output", this};
  QLabel metaLabel;
  QLineEdit moduleEdit;
  QLineEdit outputPath;

  QString baseDirDefault;
  QString requestedIpName;
  VLNV meta;
};

}  // namespace FOEDAG
