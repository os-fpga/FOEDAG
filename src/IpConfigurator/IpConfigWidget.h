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
                          const QString& moduleName = QString{},
                          const QStringList& instanceValueArgs = {});

 signals:
  void ipInstancesUpdated();

 public slots:
  void updateOutputPath();
  void handleEditorChanged(const QString& customId, QWidget* widget);

 private:
  void checkDependencies();
  void Generate(bool addToProject, const QString& outputPath = {});
  void AddDialogControls(QBoxLayout* layout);
  void AddIpToProject(const QString& cmd);
  void CreateParamFields(bool generateMetaLabel);
  void CreateOutputFields();
  void updateMetaLabel(VLNV info);
  std::vector<FOEDAG::IPDefinition*> getDefinitions();

  QMap<QVariant, QVariant> saveProperties(bool& valid) const;
  std::pair<std::string, std::string> generateNewJson(bool& ok);
  void genarateNewPanel(const std::string& newJson,
                        const std::string& filePath);
  void restoreProperties(const QMap<QVariant, QVariant>& properties);
  void showInvalidParametersWarning();

  QGroupBox* paramsBox{nullptr};
  QGroupBox outputBox{"Output", this};
  QLabel metaLabel;
  QLineEdit moduleEdit;
  QLineEdit outputPath;
  QPushButton generateBtn;

  const QString m_baseDirDefault;
  const QString m_requestedIpName;
  const QStringList m_instanceValueArgs;
  VLNV m_meta;
  QVBoxLayout* containerLayout{nullptr};
};

}  // namespace FOEDAG
