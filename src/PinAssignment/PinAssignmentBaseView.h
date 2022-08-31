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

#include <QTreeWidget>

class QComboBox;
namespace FOEDAG {

class PinsBaseModel;

/*!
 * \brief The PinAssignmentBaseView class
 * The implemenation provide common funtionality to Package pin table and Ports
 * table.
 */
class PinAssignmentBaseView : public QTreeWidget {
 public:
  PinAssignmentBaseView(PinsBaseModel *model, QWidget *parent = nullptr);

 protected:
  void removeDuplications(const QString &text, QComboBox *current);

 protected:
  PinsBaseModel *m_model{nullptr};
  bool m_blockUpdate{false};
  QVector<QComboBox *> m_allCombo;
};

}  // namespace FOEDAG
