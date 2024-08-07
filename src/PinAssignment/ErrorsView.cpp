#include "ErrorsView.h"
#include "ErrorsModel.h"

#include <QTableView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>

namespace FOEDAG {

ErrorsView::ErrorsView(ErrorsModel* model, QWidget* parent)
: QWidget(parent),
    m_tableView(new QTableView),
    m_sourceModel(model),
    m_proxyModel(new QSortFilterProxyModel(this))
{
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(1);
    setLayout(layout);

    QPushButton* openFileBn = new QPushButton;
    openFileBn->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    openFileBn->setFixedSize(22,22);
    openFileBn->setToolTip(tr("Open pcf file for edit"));
    connect(openFileBn, &QPushButton::clicked, this, &ErrorsView::openFileRequested);

    QWidget* toolBar = new QWidget;
    QVBoxLayout* toolBarLayout = new QVBoxLayout;
    toolBar->setLayout(toolBarLayout);
    toolBarLayout->setContentsMargins(0, 0, 0, 0);
    toolBarLayout->setSpacing(1);

    toolBarLayout->addWidget(openFileBn);
    toolBarLayout->addSpacerItem(new QSpacerItem{0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding});

    layout->addWidget(toolBar);
    layout->addWidget(m_tableView);

    m_proxyModel->setSourceModel(model);

    m_tableView->setModel(m_proxyModel);

    m_tableView->setSortingEnabled(true);
    m_tableView->sortByColumn(ErrorsModel::LINE_NUM, Qt::AscendingOrder);
}

void ErrorsView::setData(const QVector<QVector<QString>>& data)
{
    m_sourceModel->setData(data);
    m_tableView->resizeColumnsToContents();
}

} // namespace FOEDAG
