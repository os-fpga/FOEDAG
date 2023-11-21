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

#include <QDialog>

#include "IPGenerate/IPCatalog.h"
#include "Utils/sequential_map.h"

namespace Ui {
class IPDialogBox;
}
class QScrollArea;

namespace FOEDAG {

class IPDefinition;

class IPDialogBox : public QDialog {
  Q_OBJECT

 public:
  explicit IPDialogBox(QWidget* parent = nullptr,
                       const QString& requestedIpName = QString{},
                       const QString& moduleName = QString{},
                       const QStringList& instanceValueArgs = {});
  QString ModuleName() const;
  std::string ModuleNameStd() const;

 private slots:
  void OpenDocumentaion();
  void OpenIpLocation();
  void RestoreToDefaults();
  void GenerateIp();
  void handleEditorChanged(const QString& customId, QWidget* widget);

 private:
  static std::vector<IPDefinition*> getDefinitions();
  static IPDefinition* getDefinition(const std::string& name);
  static QString GenerateSummary(const std::string& newJson);
  sequential_map<QVariant, QVariant> saveProperties(bool& valid) const;
  void showInvalidParametersWarning();
  void restoreProperties(const sequential_map<QVariant, QVariant>& properties);
  void genarateNewPanel(const std::string& newJson,
                        const std::string& filePath);
  void CreateParamFields(bool generateParameres);
  std::pair<std::string, std::string> generateNewJson(bool& ok);
  bool Generate(bool addToProject, const QString& outputPath = {});
  void AddIpToProject(const QString& cmd);
  QString outPath() const;
  void LoadImage();

 private:
  Ui::IPDialogBox* ui{};
  const QString m_requestedIpName;
  QWidget* m_paramsBox{nullptr};
  VLNV m_meta;
  const QStringList m_instanceValueArgs;
  QScrollArea* m_scrollArea{nullptr};
};

}  // namespace FOEDAG
