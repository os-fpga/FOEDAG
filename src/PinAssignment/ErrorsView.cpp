#include "ErrorsView.h"
#include "ErrorsModel.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

namespace FOEDAG {

ErrorsView::ErrorsView(QAbstractTableModel* model, QWidget* parent)
: QTableView(parent),
    m_proxyModel(new QSortFilterProxyModel(this))
{
    connect(model, &QAbstractTableModel::modelReset, this, [this]() {
        QMetaObject::invokeMethod(this, "resizeColumnsToContents", Qt::QueuedConnection);
    });

    m_proxyModel->setSourceModel(model);

    setModel(m_proxyModel);

    setSortingEnabled(true);
    sortByColumn(ErrorsModel::LINE_NUM, Qt::AscendingOrder);
}

} // namespace FOEDAG
