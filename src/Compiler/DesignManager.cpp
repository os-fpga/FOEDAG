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
#include "DesignManager.h"

namespace FOEDAG {
DesignManager::DesignManager() {}

Design *DesignManager::activeDesign() const { return m_activeDesign; }

void DesignManager::setActiveDesign(Design *design) { m_activeDesign = design; }

bool DesignManager::setActiveDesign(const std::string &name) {
  for (Design *design : m_designs) {
    if (design->Name() == name) {
      setActiveDesign(design);
      return true;
    }
  }
  return false;
}

const std::vector<Design *> &DesignManager::designs() const {
  return m_designs;
}

void DesignManager::appendDesign(Design *d) { m_designs.push_back(d); }
}  // namespace FOEDAG
