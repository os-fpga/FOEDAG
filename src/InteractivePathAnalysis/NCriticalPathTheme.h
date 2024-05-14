#pragma once

#include <QColor>
#include <QPalette>

/**
 * @brief
 *
 * Consolidates all relevant UI style constants here.
 */

// TODO: move to qss
class NCriticalPathTheme {
 public:
  ~NCriticalPathTheme() = default;
  static NCriticalPathTheme& instance() {
    static NCriticalPathTheme theme;
    return theme;
  }
  int borderSize() const { return m_borderSize; }
  int statusIndicatorSize() const { return m_statusIndicatorSize; }
  int iconSize() const { return m_iconSize; }
  const QPalette& selectedItemPallete() const { return m_selectedItemPallete; }
  int viewFloatingItemsOffset() const { return m_viewFloatingItemsOffset; }

  const QColor& statusIndicatorOkColor() const {
    return m_statusIndicatorOkColor;
  }
  const QColor& statusIndicatorBusyColor() const {
    return m_statusIndicatorBusyColor;
  }

 private:
  NCriticalPathTheme()
      : m_statusIndicatorOkColor{"#32ba7c"},
        m_statusIndicatorBusyColor{"#f44336"} {
    m_selectedItemPallete.setColor(QPalette::Highlight,
                                   QColor("#add8e6"));  // selection color
    m_selectedItemPallete.setColor(QPalette::HighlightedText,
                                   Qt::black);  // selection color
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
