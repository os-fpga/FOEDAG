#include "runs_grid_delegate.h"

#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "device_planner_dialog.h"

using namespace FOEDAG;

RunsGridDelegate::RunsGridDelegate(DelegateType dtype, QStringList comboxData,
                                   QObject *parent)
    : QItemDelegate(parent), m_dType(dtype), m_listComboxData(comboxData) {}

RunsGridDelegate::~RunsGridDelegate() {}
void RunsGridDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
  QItemDelegate::paint(painter, option, index);
}

QSize RunsGridDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
  return QItemDelegate::sizeHint(option, index);
}

QWidget *RunsGridDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const {
  if (index.isValid() && m_dType == DT_COMBOX) {
    QComboBox *combox = new QComboBox(parent);
    // combox->setEditable(true);
    combox->installEventFilter(const_cast<RunsGridDelegate *>(this));
    return combox;
  } else if (index.isValid() && m_dType == DT_LABLE) {
    QLabel *label = new QLabel(parent);
    QString strDevice = "";
    DevicePlannerDialog *deviceDlg = new DevicePlannerDialog(parent);
    if (deviceDlg->exec()) {
      strDevice = deviceDlg->getSelectedDevice();
    }
    deviceDlg->close();
    if (strDevice != "") {
      label->setText(strDevice);
      label->setAutoFillBackground(true);
    }
    return label;
  } else {
    return QItemDelegate::createEditor(parent, option, index);
  }
}

void RunsGridDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const {
  if (index.isValid() && m_dType == DT_COMBOX) {
    QString value = index.model()->data(index, Qt::DisplayRole).toString();
    QComboBox *combox = static_cast<QComboBox *>(editor);
    foreach (QString str, m_listComboxData) { combox->addItem(str); }
    combox->setCurrentText(value);
  } else if (index.isValid() && m_dType == DT_LABLE) {
    return;
  } else {
    QItemDelegate::setEditorData(editor, index);
  }
}

void RunsGridDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  if (index.isValid() && m_dType == DT_COMBOX) {
    QComboBox *combox = static_cast<QComboBox *>(editor);
    model->setData(index, combox->currentText());
  } else if (index.isValid() && m_dType == DT_LABLE) {
    QLabel *label = static_cast<QLabel *>(editor);
    QString str = label->text();
    if (str != "") {
      model->setData(index, str);
    }
  } else {
    QItemDelegate::setModelData(editor, model, index);
  }
}

void RunsGridDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const {
  Q_UNUSED(index);
  editor->setGeometry(option.rect);
}
