#ifndef RUNSGRIDDELEGATE_H
#define RUNSGRIDDELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QWidget>

namespace FOEDAG {

enum DelegateType { DT_COMBOX, DT_LABLE };

class RunsGridDelegate : public QItemDelegate {
  Q_OBJECT
 public:
  explicit RunsGridDelegate(DelegateType dtype,
                            QStringList comboxData = QStringList(),
                            QObject *parent = nullptr);
  ~RunsGridDelegate();

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
  QSize sizeHint(const QStyleOptionViewItem &option,
                 const QModelIndex &index) const override;

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;

 private:
  DelegateType m_dType;
  QStringList m_listComboxData;
};
}  // namespace FOEDAG
#endif  // RUNSGRIDDELEGATE_H
