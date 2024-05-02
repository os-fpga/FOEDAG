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
#include <cmath>

#include "Utils/FileUtils.h"
#include "nlohmann_json/json.hpp"
using json = nlohmann::ordered_json;

namespace FOEDAG {

const QChar dspBramSep{','};

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
  auto buildLines = [](const QString &line, const std::vector<int> &columns,
                       QStringList &customLayout) -> std::pair<bool, QString> {
    auto separated = line.split(":");
    if (separated.size() < 2)
      return {false, QString{"Template file is corrupted"}};
    QString templateLine = separated.at(1);
    templateLine = templateLine.mid(0, templateLine.indexOf("/>") + 2);
    for (const auto &startx : columns) {
      QString newLine = templateLine;
      newLine.replace("${STARTX}", QString::number(startx));
      customLayout.append(newLine + "\n");
    }
    return {true, QString{}};
  };
  EFpgaMath math{m_data.eFpga};
  while (!templateFile.atEnd()) {
    QString line = templateFile.readLine();
    if (line.contains("${NAME}")) {
      line.replace("${NAME}", m_data.name);
      line.replace("${WIDTH}", QString::number(math.columns() + 4));
      line.replace("${HEIGHT}", QString::number(math.height() + 4));
    }
    if (line.contains("template_bram")) {
      auto res = buildLines(line, math.bramColumns(), customLayout);
      if (!res.first) return res;
    } else if (line.contains("template_dsp")) {
      auto res = buildLines(line, math.dspColumns(), customLayout);
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
  std::filesystem::create_directories(basePath, ec);
  if (ec) qWarning() << ec.message().c_str();
  auto layoutFile = basePath / (fileName.toStdString() + ".xml");
  QString layoutFileAsQString = QString::fromStdString(layoutFile.string());
  QFile newFile{layoutFileAsQString};
  if (newFile.open(QFile::WriteOnly)) {
    newFile.write(content.toLatin1());
    newFile.close();
  } else {
    return {false,
            QString{"Failed to create file %1"}.arg(layoutFileAsQString)};
  }

  json config = {
      {"ar", m_data.eFpga.aspectRatio}, {"bram", m_data.eFpga.bram},
      {"dsp", m_data.eFpga.dsp},        {"le", m_data.eFpga.le},
      {"fle", m_data.eFpga.fle},        {"clb", m_data.eFpga.clb},
  };
  auto configFile = basePath / (fileName.toStdString() + ".json");
  QFile userConfig{QString::fromStdString(configFile.string())};
  if (userConfig.open(QFile::WriteOnly)) {
    userConfig.write(config.dump(4).c_str());
    userConfig.close();
  } else {
    return {false, QString{"Failed to create file %1"}.arg(
                       QString::fromStdString(configFile.string()))};
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
  if (!EFpgaMath{m_data.eFpga}.isBlockCountValid()) {
    return {false, "Invalid parameters"};
  }
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
        modifyDeviceData(element, m_data.eFpga);
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
  if (!EFpgaMath{m_data.eFpga}.isBlockCountValid()) {
    return {false, "Invalid parameters"};
  }
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      auto name = e.attribute("name");
      if (name == modifyDev) {
        e.setAttribute("name", m_data.name);
        modifyDeviceData(e, m_data.eFpga);
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

  auto configFile = layoutsPath / (device.toStdString() + ".json");
  FileUtils::removeFile(configFile);
  return {true, {}};
}

std::pair<bool, QString> CustomLayoutBuilder::fromFile(
    const QString &file, const QString &deviceListFile,
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
    QString configFileName = file.chopped(4) + ".json";
    QFile configFile{configFileName};
    if (configFile.open(QFile::ReadOnly)) {
      json config;
      try {
        config.update(json::parse(configFile.readAll().toStdString()), true);
        config["ar"].get_to(data.eFpga.aspectRatio);
        config["bram"].get_to(data.eFpga.bram);
        config["dsp"].get_to(data.eFpga.dsp);
        config["clb"].get_to(data.eFpga.clb);
        config["fle"].get_to(data.eFpga.fle);
        config["le"].get_to(data.eFpga.le);
      } catch (json::parse_error &e) {
        return {false, QString{"Failed to parse file %1. Error: %2"}.arg(
                           configFileName, QString::fromLatin1(e.what()))};
      }
    } else {
      return {false, QString{"Failed to open file %1"}.arg(configFileName)};
    }
  }
  QFile devices{deviceListFile};
  if (!devices.open(QFile::ReadOnly)) {
    return {false, QString{"Failed to open file %1"}.arg(deviceListFile)};
  }
  QDomDocument devicesDoc{};
  if (!devicesDoc.setContent(&devices)) {
    devices.close();
    return {false, QString{"Incorrect device file: %1"}.arg(deviceListFile)};
  }
  devices.close();

  QDomElement docElement = devicesDoc.documentElement();
  QDomNode node = docElement.firstChild();
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();

      QString name = e.attribute("name");
      if (name == data.name) {
        QDomNodeList list = e.childNodes();
        for (int i = 0; i < list.count(); i++) {
          QDomNode n = list.at(i);
          if (!n.isNull() && n.isElement()) {
            if (n.nodeName() == "internal") {
              QString fileType = n.toElement().attribute("type");
              QString nameInternal = n.toElement().attribute("name");
              if (fileType == "base_device") {
                data.baseName = nameInternal;
                break;
              }
            }
          }
        }
        break;
      }
    }
    node = node.nextSibling();
  }
  return {true, {}};
}

void CustomLayoutBuilder::modifyDeviceData(const QDomElement &e,
                                           const EFpga &deviceResources) const {
  EFpgaMath math{deviceResources};
  auto deviceData = e.childNodes();
  for (int i = 0; i < deviceData.count(); i++) {
    if (deviceData.at(i).nodeName() == "internal") {
      auto attr = deviceData.at(i).attributes();
      auto type = attr.namedItem("type");
      if (!type.isNull() && type.toAttr().value() == "device_size") {
        auto name = attr.namedItem("name");
        if (!name.isNull()) name.setNodeValue(m_data.name);
      } else if (!type.isNull() && type.toAttr().value() == "base_device") {
        auto base_device = attr.namedItem("name");
        if (!base_device.isNull()) base_device.setNodeValue(m_data.baseName);
      }
    } else if (deviceData.at(i).nodeName() == "resource") {
      auto attr = deviceData.at(i).attributes();
      auto type = attr.namedItem("type");
      if (!type.isNull()) {
        if (type.toAttr().value() == "lut") {
          auto num = attr.namedItem("num");
          if (!num.isNull())
            num.setNodeValue(QString::number(math.lutCount(), 10));
        } else if (type.toAttr().value() == "ff") {
          auto num = attr.namedItem("num");
          if (!num.isNull())
            num.setNodeValue(QString::number(math.ffCount(), 10));
        } else if (type.toAttr().value() == "bram") {
          auto num = attr.namedItem("num");
          if (!num.isNull())
            num.setNodeValue(QString::number(math.bramCount(), 10));
        } else if (type.toAttr().value() == "dsp") {
          auto num = attr.namedItem("num");
          if (!num.isNull())
            num.setNodeValue(QString::number(math.dspCount(), 10));
        } else if (type.toAttr().value() == "carry_length") {
          auto num = attr.namedItem("num");
          if (!num.isNull())
            num.setNodeValue(QString::number(math.carryLengthCount(), 10));
        }
      }
    }
  }
}

int EFpgaMath::widthMultiple() const { return 2; }

int EFpgaMath::heightMultiple() const {
  return (m_eFpga.bram + m_eFpga.dsp) > 0 ? 6 : 2;
}

double EFpgaMath::phisicalClb() const {
  return (m_eFpga.le * 8) / 1.4 + m_eFpga.fle / 8.0 + m_eFpga.clb;
}

double EFpgaMath::grids() const {
  return m_eFpga.dsp * 3 + m_eFpga.bram * 3 + phisicalClb();
}

int EFpgaMath::width() const {
  return widthMultiple() *
         static_cast<int>(
             std::ceil((std::sqrt(grids()) / std::sqrt(m_eFpga.aspectRatio)) /
                       (double)widthMultiple()));
}

int EFpgaMath::height() const {
  return heightMultiple() *
         static_cast<int>(
             std::ceil((std::sqrt(grids()) * std::sqrt(m_eFpga.aspectRatio)) /
                       (double)heightMultiple()));
}

int EFpgaMath::dspCol() const {
  return 2 * static_cast<int>(
                 std::ceil((double)m_eFpga.dsp / (height() / 3.0) / 2.0));
}

int EFpgaMath::bramCol() const {
  return 2 * static_cast<int>(
                 std::ceil((double)m_eFpga.bram / (height() / 3.0) / 2.0));
}

int EFpgaMath::clbCol() const {
  return 2 * static_cast<int>(std::ceil((phisicalClb() / height()) / 2.0));
}

int EFpgaMath::columns() const { return dspCol() + bramCol() + clbCol(); }

int EFpgaMath::colSep() const { return 3; }

int EFpgaMath::colSepmid() const {
  return 2 * static_cast<int>(std::ceil(colSep() / 2.0));
}

int EFpgaMath::dspCount() const { return dspCol() * height() / 3; }

int EFpgaMath::bramCount() const { return bramCol() * height() / 3; }

int EFpgaMath::clbCount() const { return clbCol() * height(); }

double EFpgaMath::need() const { return 1 - (1.0 / double(1 + colSep())); }

double EFpgaMath::actual() const {
  return (clbCol() - colSepmid()) / (double)(columns() - colSepmid());
}

bool EFpgaMath::isBlockCountValid() const { return actual() > need(); }

bool EFpgaMath::isLutCountValid() const { return lutCount() > 0; }

bool EFpgaMath::isDeviceSizeValid() const {
  auto w = columns();
  auto h = height();
  if (w < 1 || w > 160) return false;
  if (h < 1 || h > 160) return false;
  return true;
}

int EFpgaMath::lutCount() const { return clbCount() * 8; }

int EFpgaMath::ffCount() const { return lutCount() * 2; }

int EFpgaMath::carryLengthCount() const { return height() * 2; }

std::vector<int> EFpgaMath::dspColumns() const { return layoutFor(DSP); }

std::vector<int> EFpgaMath::bramColumns() const { return layoutFor(BRAM); }

std::vector<int> EFpgaMath::layoutHalf() const {
  std::vector<int> half;
  int bram = bramCol() / 2;
  int dsp = dspCol() / 2;
  half.push_back(CLB);
  half.push_back(CLB);
  while ((bram > 0) || (dsp > 0)) {
    if (bram > 0) {
      half.push_back(BRAM);
      half.push_back(CLB);
      half.push_back(CLB);
      half.push_back(CLB);
      bram--;
    }
    if (dsp > 0) {
      half.push_back(DSP);
      half.push_back(CLB);
      half.push_back(CLB);
      half.push_back(CLB);
      dsp--;
    }
  }
  return half;
}

std::vector<int> EFpgaMath::layoutFor(Blocks block) const {
  auto half = layoutHalf();
  int startPos = (columns() + 4) / 2;  // we taking offset into account
  std::vector<int> columns{};
  for (int i = 0; i < half.size(); i++) {
    if (half.at(i) == block) {
      columns.push_back(startPos + i);
      columns.push_back(startPos - i - 1);  // fliped layout here
    }
  }
  std::sort(columns.begin(), columns.end());
  return columns;
}

}  // namespace FOEDAG
