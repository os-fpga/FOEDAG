#pragma once

#include <QStyledItemDelegate>

class NCriticalPathItemDelegate final : public QStyledItemDelegate {
  Q_OBJECT
 public:
  NCriticalPathItemDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate(parent) {}
  ~NCriticalPathItemDelegate() override final = default;

 protected:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override final;

 private:
  int m_border = 2;
};
