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
#include "ip_configuration.h"

#include <algorithm>

namespace FOEDAG {

const std::vector<std::string> &IpConfiguration::ipCatalogPathList() const {
  return m_ipCatalogPathList;
}

void IpConfiguration::setIpCatalogPathList(
    const std::vector<std::string> &newIpCatalogPathList) {
  m_ipCatalogPathList = newIpCatalogPathList;
}

void IpConfiguration::addIpCatalogPath(const std::string &ipCatalogPath) {
  auto find = std::find(m_ipCatalogPathList.begin(), m_ipCatalogPathList.end(),
                        ipCatalogPath);
  if (find == m_ipCatalogPathList.end())
    m_ipCatalogPathList.push_back(ipCatalogPath);
}

const std::vector<std::string> &IpConfiguration::instancePathList() const {
  return m_instancePathList;
}

void IpConfiguration::setInstancePathList(
    const std::vector<std::string> &newInstancePathList) {
  m_instancePathList = newInstancePathList;
}

void IpConfiguration::addInstancePath(const std::string &instancePath) {
  auto find = std::find(m_instancePathList.begin(), m_instancePathList.end(),
                        instancePath);
  if (find == m_instancePathList.end())
    m_instancePathList.push_back(instancePath);
}

const std::vector<std::string> &IpConfiguration::instanceCmdList() const {
  return m_instanceCmdList;
}

void IpConfiguration::setInstanceCmdList(
    const std::vector<std::string> &newInstanceCmdList) {
  m_instanceCmdList = newInstanceCmdList;
}

void IpConfiguration::addInstanceCmd(const std::string &instanceCmd) {
  auto find = std::find(m_instanceCmdList.begin(), m_instanceCmdList.end(),
                        instanceCmd);
  if (find == m_instanceCmdList.end()) m_instanceCmdList.push_back(instanceCmd);
}

}  // namespace FOEDAG
