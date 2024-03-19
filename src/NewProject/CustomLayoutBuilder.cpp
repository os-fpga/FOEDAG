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

#include "Utils/FileUtils.h"

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

std::pair<bool, QString> CustomLayoutBuilder::saveCustomLayout(
    const std::filesystem::path &basePath, const QString &fileName,
    const QString &content) {
  std::error_code ec;
  // make sure directory exists
  std::filesystem::create_directory(basePath, ec);
  auto layoutFile = basePath / fileName.toStdString();
  QString layoutFileAsQString = QString::fromStdString(layoutFile.string());
  QFile newFile{layoutFileAsQString};
  if (newFile.open(QFile::WriteOnly)) {
    newFile.write(content.toLatin1());
    newFile.close();
  } else {
    return {false,
            QString{"Failed to create file %1"}.arg(layoutFileAsQString)};
  }
  return {true, {}};
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
        auto baseDevNode = newDoc.createElement("internal");
        baseDevNode.setAttribute("type", "base_device");
        baseDevNode.setAttribute("name", baseDevice);
        element.appendChild(baseDevNode);
        QDomElement deviceElem = root.lastChildElement("device");
        QDomNode newNode{};
        if (deviceElem.isNull()) {
          newNode = root.appendChild(element);
        } else {
          newNode = root.insertAfter(element, deviceElem);
        }
        if (!newNode.isNull()) {
          QTextStream stream;
          targetDevice.resize(0);
          stream.setDevice(&targetDevice);
          newDoc.save(stream, 4);
          targetDevice.close();
          return {true, QString{}};
        }
        return {false, "Failed to modify custom device list"};
      }
    }
    node = node.nextSibling();
  }
  return {true, QString{}};
}

std::pair<bool, QString> CustomLayoutBuilder::modifyDevice(
    const QString &targetDeviceXml, const QString &modifyDev) const {
  QFile file(targetDeviceXml);
  if (!file.open(QFile::ReadWrite)) {
    return {false, QString{"Cannot open device file: %1"}.arg(targetDeviceXml)};
  }
  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    return {false, QString{"Incorrect device file: %1"}.arg(targetDeviceXml)};
  }

  QDomElement docElement = doc.documentElement();
  QDomNode node = docElement.firstChild();
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      auto name = e.attribute("name");
      if (name == modifyDev) {
        e.setAttribute("name", m_data.name);
        auto deviceData = e.childNodes();
        for (int i = 0; i < deviceData.count(); i++) {
          if (deviceData.at(i).nodeName() == "internal") {
            auto attr = deviceData.at(i).attributes();
            auto type = attr.namedItem("type");
            if (!type.isNull() && type.toAttr().value() == "device_size") {
              auto name = attr.namedItem("name");
              if (!name.isNull()) name.setNodeValue(m_data.name);
            } else if (!type.isNull() &&
                       type.toAttr().value() == "base_device") {
              auto base_device = attr.namedItem("name");
              if (!base_device.isNull())
                base_device.setNodeValue(m_data.baseName);
            }
          }
        }
        QTextStream stream;
        file.resize(0);
        stream.setDevice(&file);
        doc.save(stream, 4);
        file.close();
        return {true, QString{}};
      }
    }
    node = node.nextSibling();
  }
  file.close();
  return {false, QString{"Failed to find custom device %1"}.arg(modifyDev)};
}

std::pair<bool, QString> CustomLayoutBuilder::removeDevice(
    const QString &deviceXml, const std::filesystem::path &layoutsPath,
    const QString &device) {
  QFile targetDevice{deviceXml};
  if (!targetDevice.open(QFile::ReadWrite)) {
    return {false, "Failed to open custom_device.xml"};
  }
  QDomDocument newDoc{};
  newDoc.setContent(&targetDevice);
  QDomElement root = newDoc.firstChildElement("device_list");
  if (!root.isNull()) {
    auto devices = root.childNodes();
    bool deviceRemoved{false};
    for (int i = 0; i < devices.count(); i++) {
      auto attr = devices.at(i).attributes();
      if (attr.contains("name") &&
          attr.namedItem("name").toAttr().value() == device) {
        root.removeChild(devices.at(i));
        deviceRemoved = true;
        break;
      }
    }
    if (deviceRemoved) {
      QTextStream stream;
      targetDevice.resize(0);
      stream.setDevice(&targetDevice);
      newDoc.save(stream, 4);
      targetDevice.close();
    }
  }
  // remove layout file <custom device name>.xml
  auto layoutFile = layoutsPath / (device.toStdString() + ".xml");
  FileUtils::removeFile(layoutFile);
  return {true, {}};
}

std::pair<bool, QString> CustomLayoutBuilder::fromFile(const QString &file,
                                                       CustomLayoutData &data) {
  QFile customLayout{file};
  if (!customLayout.open(QFile::ReadOnly)) {
    return {false, QString{"Failed to open file %1"}.arg(file)};
  }
  QDomDocument doc{};
  doc.setContent(&customLayout);
  auto root = doc.documentElement();
  if (!root.isNull()) {
    if (root.nodeName() != "fixed_layout") {
      return {false, QString{"Failed to find \"fixed_layout\" tag"}};
    }
    if (root.hasAttribute("name")) {
      data.name = root.attribute("name");
    } else {
      return {false, "Failed to find \"name\" attribute"};
    }
    if (root.hasAttribute("width")) {
      bool ok{false};
      auto width = root.attribute("width").toInt(&ok, 10);
      if (ok) data.width = width;
    } else {
      return {false, "Failed to find \"width\" attribute"};
    }
    if (root.hasAttribute("height")) {
      bool ok{false};
      auto height = root.attribute("height").toInt(&ok, 10);
      if (ok) data.height = height;
    } else {
      return {false, "Failed to find \"height\" attribute"};
    }
    auto children = root.childNodes();
    QStringList dsp;
    QStringList bram;
    for (int i = 0; i < children.count(); i++) {
      if (children.at(i).nodeName() == "col") {
        auto e = children.at(i).toElement();
        if (!e.isNull()) {
          if (e.hasAttribute("type")) {
            if (e.attribute("type") == "dsp") {
              dsp.append(e.attribute("startx"));
            } else if (e.attribute("type") == "bram")
              bram.append(e.attribute("startx"));
          }
        }
      }
    }
    data.bram = bram.join(",");
    data.dsp = dsp.join(",");
  } else {
    return {false, QString{"Failed to load %1"}.arg(file)};
  }
  return {true, {}};
}

}  // namespace FOEDAG
