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
#include "CustomLayoutBuilder.h"

#include <QDebug>
#include <QDomDocument>
#include <QFile>

namespace FOEDAG {

CustomLayoutBuilder::CustomLayoutBuilder(const CustomLayoutData &data,
                                         const QString &templateLayout)
    : m_data(data), m_templateLayout(templateLayout) {}

std::pair<bool, QString> CustomLayoutBuilder::testTemplateFile() const {
  QFile templateFile{m_templateLayout};
  if (!templateFile.open(QFile::ReadOnly))
    return {false,
            QString{"Failed to open template layout %1"}.arg(m_templateLayout)};

  return {true, {}};
}

std::pair<bool, QString> CustomLayoutBuilder::generateCustomLayout() const {
  QFile templateFile{m_templateLayout};
  if (!templateFile.open(QFile::ReadOnly)) {
    return {false,
            QString{"Failed to open template layout %1"}.arg(m_templateLayout)};
  }
  QStringList customLayout{};
  auto buildLines = [](const QString &line, const QString &userInput,
                       QStringList &customLayout) -> std::pair<bool, QString> {
    auto separated = line.split(":");
    if (separated.size() < 2)
      return {false, QString{"Template file is corrupted"}};
    QString templateLine = separated.at(1);
    templateLine = templateLine.mid(0, templateLine.indexOf("/>") + 2);
    auto columns = userInput.split(",", Qt::SkipEmptyParts);
    for (const auto &startx : columns) {
      QString newLine = templateLine;
      newLine.replace("${STARTX}", startx);
      customLayout.append(newLine + "\n");
    }
    return {true, QString{}};
  };
  while (!templateFile.atEnd()) {
    QString line = templateFile.readLine();
    if (line.contains("${NAME}")) {
      line.replace("${NAME}", m_data.name);
      line.replace("${WIDTH}", QString::number(m_data.width));
      line.replace("${HEIGHT}", QString::number(m_data.height));
    }
    if (line.contains("template_bram")) {
      auto res = buildLines(line, m_data.bram, customLayout);
      if (!res.first) return res;
    } else if (line.contains("template_dsp")) {
      auto res = buildLines(line, m_data.dsp, customLayout);
      if (!res.first) return res;
    } else {
      customLayout.append(line);
    }
  }

  return {true, customLayout.join("")};
}

std::pair<bool, QString> CustomLayoutBuilder::generateNewDevice(
    const QString &deviceXml, const QString &targetDeviceXml,
    const QString &baseDevice) const {
  if (baseDevice.isEmpty()) return {false, "No device selected"};
  QFile file(deviceXml);
  if (!file.open(QFile::ReadOnly)) {
    return {false, QString{"Cannot open device file: %1"}.arg(deviceXml)};
  }
  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    return {false, QString{"Incorrect device file: %1"}.arg(deviceXml)};
  }
  file.close();

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      auto name = e.attribute("name");
      if (name == baseDevice) {
        QDomDocument newDoc{};
        QFile targetDevice{targetDeviceXml};
        if (!targetDevice.open(QFile::ReadWrite)) {
          return {false, "Failed to open custom_device.xml"};
        }
        newDoc.setContent(&targetDevice);
        QDomElement root = newDoc.firstChildElement("device_list");
        if (root.isNull()) {  // new file
          root = newDoc.createElement("device_list");
          newDoc.appendChild(root);
        }
        auto copy = newDoc.importNode(node, true);
        auto element = copy.toElement();
        element.setAttribute("name", m_data.name);
        auto deviceData = element.childNodes();
        for (int i = 0; i < deviceData.count(); i++) {
          if (deviceData.at(i).nodeName() == "internal") {
            auto attr = deviceData.at(i).attributes();
            auto type = attr.namedItem("type");
            if (!type.isNull() && type.toAttr().value() == "device_size") {
              auto name = attr.namedItem("name");
              if (!name.isNull()) name.setNodeValue(m_data.name);
              break;
            }
          }
        }
        QDomElement deviceElem = root.lastChildElement("device");
        if (deviceElem.isNull()) {
          root.appendChild(element);
        } else {
          root.insertAfter(element, deviceElem);
        }
        QTextStream stream;
        targetDevice.resize(0);
        stream.setDevice(&targetDevice);
        newDoc.save(stream, 4);
        file.close();
        break;
      }
    }
    node = node.nextSibling();
  }
  return {true, QString{}};
}

void CustomLayoutBuilder::setCustomLayoutData(const CustomLayoutData &data) {
  m_data = data;
}

}  // namespace FOEDAG
