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
#include <QObject>
#include <vector>

#include "Compiler/Design.h"

namespace FOEDAG {

class DesignManager : public QObject {
  Q_OBJECT
 public:
  DesignManager();
  Design *activeDesign() const;
  void setActiveDesign(Design *design);
  bool setActiveDesign(const std::string &name);

  const std::vector<Design *> &designs() const;
  void appendDesign(Design *d);

 signals:
  void topLevelChanged();

 private:
  std::vector<Design *> m_designs;
  Design *m_activeDesign = nullptr;
};

}  // namespace FOEDAG
