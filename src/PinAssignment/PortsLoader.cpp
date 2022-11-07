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
#include "PortsLoader.h"

#include <QFile>

#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

namespace FOEDAG {

PortsLoader::PortsLoader(PortsModel *model, QObject *parent)
    : QObject(parent), m_model(model) {}

std::pair<bool, QString> PortsLoader::load(const QString &file) {
  if (!m_model) return std::make_pair(false, "Ports model is missing");
  QFile f{file};
  if (!f.open(QFile::ReadOnly))
    return std::make_pair(false, QString("Can't open file %1").arg(file));

  QString content = f.readAll();
  json jsonObject;
  try {
    jsonObject = json::parse(content.toStdString());
  } catch (json::parse_error &e) {
    const QString error =
        QString("Json Error: %1\nFile: %2\nByte position of error: %3")
            .arg(e.what(), file, QString::number(e.byte));
    return std::make_pair(false, error);
  }
  for (auto p{jsonObject.cbegin()}; p != jsonObject.cend(); ++p) {
    IOPortGroup group;
    auto ports = p->at("ports");
    group.ports.reserve(ports.size());
    for (auto it{ports.cbegin()}; it != ports.cend(); ++it) {
      const auto range = it->at("range");
      const int msb = range["msb"];
      const int lsb = range["lsb"];

      IOPort ioport{QString::fromStdString(it->at("name")),
                    QString::fromStdString(it->at("direction")),
                    QString(),
                    QString::fromStdString(it->at("type")),
                    QString("Msb: %1, lsb: %2")
                        .arg(QString::number(msb), QString::number(lsb)),
                    (msb != lsb),
                    {}};
      if (ioport.isBus) {
        const int step = msb > lsb ? -1 : 1;
        const int end = lsb + step;
        ioport.ports.reserve(std::abs(msb - lsb) + 1);
        for (int i{msb}; i != end; i += step) {
          const IOPort port{
              QString("%1[%2]").arg(QString::fromStdString(it->at("name")),
                                    QString::number(i)),
              QString::fromStdString(it->at("direction")),
              QString(),
              QString::fromStdString(it->at("type")),
              QString("Msb: %1, lsb: %2")
                  .arg(QString::number(msb), QString::number(lsb)),
              false,
              {}};
          ioport.ports.append(port);
        }
      }
      group.ports.append(ioport);
    }
    m_model->append(group);
  }
  m_model->initListModel();
  return std::make_pair(true, QString());
}

void PortsLoader::SetModel(PortsModel *model) { m_model = model; }

}  // namespace FOEDAG
