#include "NCriticalPathItemDelegate.h"

#include <QColor>
#include <QPainter>

void NCriticalPathItemDelegate::paint(QPainter* painter,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
  QStyleOptionViewItem newOpt{option};
  newOpt.state &=
      ~QStyle::State_HasFocus;  // remove standard focused item frame

  QStyledItemDelegate::paint(painter, newOpt, index);
  if (bool isEditable = index.data(Qt::DecorationRole).toBool()) {
    newOpt.rect.adjust(m_border, m_border, -m_border, -m_border);
    painter->fillRect(newOpt.rect, QColor(0, 0, 0, 15));
  }
}
