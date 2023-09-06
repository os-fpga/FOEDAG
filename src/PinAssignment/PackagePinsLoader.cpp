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
#include "PackagePinsLoader.h"

#include <QFile>
#include <QSet>

#include "Utils/QtUtils.h"
#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

namespace FOEDAG {

PackagePinsLoader::PackagePinsLoader(PackagePinsModel *model, QObject *parent)
    : QObject(parent), m_model(model) {}

std::pair<bool, QString> PackagePinsLoader::load(const QString &fileName) {
  const auto &[success, content] = getFileContent(fileName);
  if (!success) return std::make_pair(success, content);

  InternalPins &internalPins = m_model->internalPinsRef();
  QStringList lines = QtUtils::StringSplit(content, '\n');
  parseHeader(lines.takeFirst());
  PackagePinGroup group{};
  QSet<QString> uniquePins;
  for (const auto &line : lines) {
    QStringList data = line.split(",");
    if (!data.first().isEmpty()) {
      if (!group.name.isEmpty() && (group.name != data.first())) {
        if (m_model->userGroups().contains(group.name)) m_model->append(group);
        group.pinData.clear();
        uniquePins.clear();
      }
      group.name = data.first();
    }
    data.pop_front();
    // internal pins parsing
    for (int i = ModeFirst; (i <= ModeLast) && (i < data.count()); i++) {
      if (data.at(i) == "Y") {
        internalPins[data.at(BallName)][i].append(data.at(InternalPinName));
        m_model->insertBallData(data.at(BallName), data.at(BallId));
      }
    }
    // -------------
    if (uniquePins.contains(data.at(BallName))) continue;
    uniquePins.insert(data.at(BallName));
    group.pinData.append({data});
  }
  if (m_model->userGroups().contains(group.name))
    m_model->append(group);  // append last
  m_model->initListModel();

  return std::make_pair(true, QString());
}

std::pair<bool, QString> PackagePinsLoader::loadHeader(
    const QString &fileName) {
  const auto &[success, content] = getFileContent(fileName);
  if (!success) return std::make_pair(success, content);

  json jsonObject;
  try {
    jsonObject = json::parse(content.toStdString());
  } catch (json::parse_error &e) {
    const QString error =
        QString("Json Error: %1\nFile: %2\nByte position of error: %3")
            .arg(e.what(), fileName, QString::number(e.byte));
    return std::make_pair(false, error);
  }
  auto columns = jsonObject.at("table");
  for (const auto &c : columns) {
    int id = c.at("colmumnid");
    bool visible = c.at("visible");
    const HeaderData header{QString::fromStdString(c.at("name")),
                            QString::fromStdString(c.at("description")), id,
                            visible};
    m_model->appendHeaderData(header);
  }
  auto groups = jsonObject.at("groups");
  for (const auto &group : groups) {
    m_model->appendUserGroup(QString::fromStdString(group));
  }

  return std::make_pair(true, QString());
}

void PackagePinsLoader::setModel(PackagePinsModel *model) { m_model = model; }

std::pair<bool, QString> PackagePinsLoader::getFileContent(
    const QString &fileName) const {
  if (!m_model) return std::make_pair(false, "Package pin model is missing");
  QFile file{fileName};
  if (!file.open(QFile::ReadOnly))
    return std::make_pair(false, QString("Can't open file %1").arg(fileName));

  return std::make_pair(true, file.readAll());
}

void PackagePinsLoader::parseHeader(const QString &header) {
  const QStringList columns = header.split(",");
  QStringList modesRx{};
  QStringList modesTx{};
  for (const auto &col : columns) {
    if (col.startsWith("Mode_", Qt::CaseInsensitive)) {
      m_model->insertMode(columns.indexOf(col) - 1, col);
      if (col.endsWith("tx", Qt::CaseInsensitive))
        modesTx.append(col);
      else if (col.endsWith("rx", Qt::CaseInsensitive))
        modesRx.append(col);
    }
  }

  if (!modesRx.isEmpty()) modesRx.push_front({});  // one empty element
  if (!modesTx.isEmpty()) modesTx.push_front({});  // one empty element

  m_model->modeModelTx()->setStringList(modesTx);
  m_model->modeModelRx()->setStringList(modesRx);
}

}  // namespace FOEDAG
