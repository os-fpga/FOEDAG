#pragma once

#include <QStyledItemDelegate>

class NCriticalPathItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    NCriticalPathItemDelegate(QObject* parent = nullptr): QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


