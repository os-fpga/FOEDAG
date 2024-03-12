/**
  * @file NCriticalPathTheme.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
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

#include <QPalette>
#include <QColor>

namespace FOEDAG {

/**
 * @brief 
 * 
 * Consolidates all relevant UI style constants here.
 */

// TODO: move to qss
class NCriticalPathTheme
{
public:
    ~NCriticalPathTheme()=default;
    static NCriticalPathTheme& instance() {
        static NCriticalPathTheme theme;
        return theme;
    }
    int borderSize() const { return m_borderSize; }
    int statusIndicatorSize() const { return m_statusIndicatorSize; }
    int iconSize() const { return m_iconSize; }
    const QPalette& selectedItemPallete() const { return m_selectedItemPallete; }
    int viewFloatingItemsOffset() const { return m_viewFloatingItemsOffset; }

    const QColor& statusIndicatorOkColor() const { return m_statusIndicatorOkColor; }
    const QColor& statusIndicatorBusyColor() const { return m_statusIndicatorBusyColor; }

private:
    NCriticalPathTheme():
        m_statusIndicatorOkColor{"#32ba7c"}
        , m_statusIndicatorBusyColor{"#f44336"}
    {
        m_selectedItemPallete.setColor(QPalette::Highlight, QColor("#add8e6")); // selection color
        m_selectedItemPallete.setColor(QPalette::HighlightedText, Qt::black); // selection color
    };

 private:
    int m_borderSize = 5;
    int m_statusIndicatorSize = 12;
    int m_iconSize = 20;
    int m_viewFloatingItemsOffset = 5;
    QPalette m_selectedItemPallete;

    QColor m_statusIndicatorOkColor;
    QColor m_statusIndicatorBusyColor;
};

} // namespace FOEDAG