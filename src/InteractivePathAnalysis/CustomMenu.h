/**
  * @file CustomMenu.h
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

#include <QMouseEvent>
#include <QWidget>

class QPushButton;
class QVBoxLayout;

namespace FOEDAG {

/**
 * @brief Simple Menu Implementation
 *
 * Behaves like a regular menu but allows any type of widgets in the menu body.
 */
class CustomMenu final : public QWidget {
  Q_OBJECT
 public:
  explicit CustomMenu(QPushButton* caller);

  void addContentLayout(QLayout*);
  void setLayout(QLayout* layout) = delete;

  void setAlignment(Qt::Alignment alignment) { m_alignment = alignment; }
  void show() = delete;
  void popup(QPoint pos);

  void setButtonToolTips(const QString& toolTipForDoneButton,
                         const QString& toolTipForCancelButton);

 signals:
  void accepted();
  void declined();

 protected:
  void mousePressEvent(QMouseEvent* event) override final;
  void hideEvent(QHideEvent* event) override final;

 private:
  bool m_isAccepted = false;
  Qt::Alignment m_alignment = Qt::AlignLeft;
  QVBoxLayout* m_contentLayout = nullptr;

  QPushButton* m_bnCancel = nullptr;
  QPushButton* m_bnDone = nullptr;
};

}  // namespace FOEDAG