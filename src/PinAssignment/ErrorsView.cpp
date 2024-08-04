#include "ErrorsView.h"

#include <QAbstractTableModel>

namespace FOEDAG {

ErrorsView::ErrorsView(QAbstractTableModel* model, QWidget* parent)
: QTableView(parent)
{
    connect(model, &QAbstractTableModel::modelReset, this, [this]() {
        QMetaObject::invokeMethod(this, "resizeColumnsToContents", Qt::QueuedConnection);
    });

    setModel(model);
}

} // namespace FOEDAG
