#include "ErrorsView.h"
#include "ErrorsModel.h"

#include <QSortFilterProxyModel>

namespace FOEDAG {

ErrorsView::ErrorsView(ErrorsModel* model, QWidget* parent)
: QTableView(parent),
    m_sourceModel(model),
    m_proxyModel(new QSortFilterProxyModel(this))
{
    m_proxyModel->setSourceModel(model);

    setModel(m_proxyModel);

    setSortingEnabled(true);
    sortByColumn(ErrorsModel::LINE_NUM, Qt::AscendingOrder);
}

void ErrorsView::setData(const QVector<QVector<QString>>& data)
{
    m_sourceModel->setData(data);
    resizeColumnsToContents();
}

} // namespace FOEDAG
