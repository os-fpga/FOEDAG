#pragma once

#include <QStyledItemDelegate>

class NCriticalPathItemDelegate final: public QStyledItemDelegate
{
    Q_OBJECT
public:
    NCriticalPathItemDelegate(QObject* parent = nullptr): QStyledItemDelegate(parent) {}
    ~NCriticalPathItemDelegate() override final=default;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override final;
};


