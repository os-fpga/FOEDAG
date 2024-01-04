#include "NCriticalPathItemDelegate.h"

#include <QPainter>
#include <QColor>

void NCriticalPathItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem newOpt{option};
    newOpt.state &= ~QStyle::State_HasFocus; // remove standard focused item frame
    QStyledItemDelegate::paint(painter, newOpt, index);

    QVariant data = index.data(Qt::DecorationRole);

    if (data.toBool()) {
        painter->fillRect(newOpt.rect, QColor(0, 0, 0, 15));
    }
}
