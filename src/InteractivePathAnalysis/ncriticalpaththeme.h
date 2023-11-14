#pragma once

#include <QPalette>

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
    const QPalette& selectedItemPallete() const { return m_selectedItemPallete; }
    int viewFloatingItemsOffset() const { return m_viewFloatingItemsOffset; }
private:
    NCriticalPathTheme() {
        m_selectedItemPallete.setColor(QPalette::Highlight, QColor("#ADD8E6")); // selection color
        m_selectedItemPallete.setColor(QPalette::HighlightedText, Qt::black); // selection color
    };

 private:
    int m_borderSize = 5;
    int m_statusIndicatorSize = 10;
    int m_viewFloatingItemsOffset = 10;
    QPalette m_selectedItemPallete;
};
