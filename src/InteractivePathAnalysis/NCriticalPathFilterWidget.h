/**
  * @file NCriticalPathFilterWidget.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QGroupBox>
#include <QHBoxLayout>

#include "FilterCriteriaConf.h"

class QComboBox;
class QLineEdit;
class QCheckBox;

namespace FOEDAG {

class NCriticalPathFilterWidget : public QGroupBox {
  Q_OBJECT

  struct UiBackup {
    bool isValid = false;

    int comboBoxItemIndex = -1;
    QString criteria;
    bool checkBoxCaseSensetive = false;
    bool checkBoxRegexp = false;

    void clear() {
      isValid = false;

      comboBoxItemIndex = -1;
      criteria = "";
      checkBoxCaseSensetive = false;
      checkBoxRegexp = false;
    }
  };

 public:
  explicit NCriticalPathFilterWidget(const QString& name,
                                     QWidget* parent = nullptr);
  ~NCriticalPathFilterWidget() = default;

  void fillComboBoxWithNodes(const std::map<QString, int>& data);

  FilterCriteriaConf criteriaConf() const;

  QComboBox* comboBox() const { return m_comboBox; }

  void clear();

  void onAccepted();
  void onDeclined();

 private:
  QComboBox* m_comboBox = nullptr;
  QLineEdit* m_lineEdit = nullptr;
  QCheckBox* m_chUseRegexp = nullptr;
  QCheckBox* m_chUseCaseSensetive = nullptr;

  UiBackup m_backup;

  template <typename T1, typename T2>
  QWidget* wrapIntoRowWidget(T1 w1, T2* w2) {
    QWidget* container = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout;
    container->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(w1);
    layout->addWidget(w2);
    return container;
  }

  void resetComboBoxSilently();
  void resetLineEditSilently();

  void backupUI();
  void restoreUIFromBackup();

  void resetUI();
};

}  // namespace FOEDAG