#include "NCriticalPathView.h"
#include "NCriticalPathModel.h"
#include "NCriticalPathItemDelegate.h"
#include "NCriticalPathFilterWidget.h"
#include "NCriticalPathFilterModel.h"
#include "NCriticalPathModel.h"
#include "NCriticalPathTheme.h"
#include "NCriticalPathParameters.h"
#include "NCriticalPathItem.h"
#include "CustomMenu.h"
#include "SimpleLogger.h"

#include <QScrollBar>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QMouseEvent>

#include <map>
#include <set>

NCriticalPathView::NCriticalPathView(QWidget* parent)
    : QTreeView(parent)
{
    setSelectionMode(QAbstractItemView::MultiSelection);

    setAutoScroll(false);
    setExpandsOnDoubleClick(false); // will be redirected on single mouse right button press

    setPalette(NCriticalPathTheme::instance().selectedItemPallete());

    NCriticalPathItemDelegate* customDelegate = new NCriticalPathItemDelegate(this);
    setItemDelegate(customDelegate);

    const int iconSize = NCriticalPathTheme::instance().iconSize();

    // setup expand controls
    m_bnExpandCollapse = new QPushButton(this);
    m_bnExpandCollapse->setFixedSize(iconSize,iconSize);
    QObject::connect(m_bnExpandCollapse, &QPushButton::clicked, this, [this](){
        if (m_isCollapsed) {
            expandAll();
            m_bnExpandCollapse->setIcon(QIcon(":/down-arrow.png"));
            m_isCollapsed = false;
        } else {
            collapseAll();
            m_bnExpandCollapse->setIcon(QIcon(":/next.png"));
            m_isCollapsed = true;
        }
    });

    // setup clear bn
    m_bnClearSelection = new QPushButton(this);
    m_bnClearSelection->setFixedSize(iconSize, iconSize);
    m_bnClearSelection->setIcon(QIcon(":/cross.png"));
    QObject::connect(m_bnClearSelection, &QPushButton::clicked, this, &NCriticalPathView::clearSelection);

    setupFilterMenu();

    hideControls();
}

void NCriticalPathView::mousePressEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        if (event->button() == Qt::RightButton) {
            if (isExpanded(index)) {
                collapse(index);
            } else {
                expand(index);
            }
            return;
        }
    }

    QTreeView::mousePressEvent(event);
}

void NCriticalPathView::setModel(QAbstractItemModel* proxyModel)
{
    QTreeView::setModel(proxyModel);

    m_filterModel = qobject_cast<NCriticalPathFilterModel*>(proxyModel);
    if (m_filterModel) {
        m_sourceModel = qobject_cast<NCriticalPathModel*>(m_filterModel->sourceModel());
    }    

    // selectionModel() is null before we set the model, that's why we create the connection after model set
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &NCriticalPathView::handleSelection);
}

void NCriticalPathView::handleSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (!m_sourceModel) {
        return;
    }

    bool skipReportingSelectionChangeOnThisTurn = false;

    for (const QModelIndex& index: selected.indexes()) {
        if (index.isValid()) {
            QString data{index.data(Qt::DisplayRole).toString()};
            NCriticalPathItem* item = m_sourceModel->getItemByData(data);
            if (item) {
                if (item->isPath()) {
                    if (updateChildrenSelectionFor(item, true)) {
                        skipReportingSelectionChangeOnThisTurn = true;
                    }
                }
            }
        }
    }

    for (const QModelIndex& index: deselected.indexes()) {
        if (index.isValid()) {
            QString data{index.data(Qt::DisplayRole).toString()};
            NCriticalPathItem* item = m_sourceModel->getItemByData(data);
            if (item) {
                if (item->isPath()) {
                    if (updateChildrenSelectionFor(item, false)) {
                        skipReportingSelectionChangeOnThisTurn = true;
                    }
                }
            }
        }
    }

    if (!skipReportingSelectionChangeOnThisTurn) {
        QString selectedPathElements = getSelectedPathElements();
        SimpleLogger::instance().log("selectedPathElements:", selectedPathElements);
        m_bnClearSelection->setVisible(!selectedPathElements.isEmpty());
        emit pathElementSelectionChanged(selectedPathElements, "selectedPathElements");
    }
}

void NCriticalPathView::fillInputOutputData(const std::map<QString, int>& inputs, const std::map<QString, int>& outputs)
{
    m_inputFilter->fillComboBoxWithNodes(inputs);
    m_outputFilter->fillComboBoxWithNodes(outputs);

    // set equal minimum sizes for both comboboxes depending on the max content width
    int inputWidth = m_inputFilter->comboBox()->sizeHint().width();
    int outputWidth = m_outputFilter->comboBox()->sizeHint().width();
    int maxWidth = std::max(inputWidth, outputWidth);
    m_inputFilter->comboBox()->setMinimumWidth(maxWidth);
    m_outputFilter->comboBox()->setMinimumWidth(maxWidth);
}

void NCriticalPathView::setupFilterMenu()
{
    if (m_filterMenu) {
        return;
    }
    m_bnFilter = new QPushButton(this);
    int iconSize = NCriticalPathTheme::instance().iconSize();
    m_bnFilter->setFixedSize(iconSize,iconSize);
    m_bnFilter->setIcon(QIcon(":/search.png"));
    m_filterMenu = new CustomMenu(m_bnFilter);
    m_filterMenu->setAlignment(CustomMenu::Alignment::RIGHT);

    QVBoxLayout* layout = new QVBoxLayout;
    m_filterMenu->addContentLayout(layout);

    m_inputFilter = new NCriticalPathFilterWidget(tr("Input Nodes:"));
    m_outputFilter = new NCriticalPathFilterWidget(tr("Output Nodes:"));

    connect(m_filterMenu, &CustomMenu::accepted, this, [this](){
        m_inputFilter->onAccepted();
        m_outputFilter->onAccepted();
        emit criteriaFilterChanged(m_inputFilter->criteriaConf(), m_outputFilter->criteriaConf());
    });
    connect(m_filterMenu, &CustomMenu::declined, this, [this](){
        m_inputFilter->onDeclined();
        m_outputFilter->onDeclined();
    });
    layout->addWidget(m_inputFilter);
    layout->addWidget(m_outputFilter);
}

void NCriticalPathView::hideControls()
{
    m_bnExpandCollapse->setIcon(QIcon(":/next.png"));
    m_bnExpandCollapse->setVisible(false);
    m_isCollapsed = true;

    m_bnClearSelection->setVisible(false);

    m_bnFilter->setVisible(false);
}

void NCriticalPathView::onDataLoaded()
{
    m_bnExpandCollapse->setVisible(true);
    m_bnFilter->setVisible(true);
}

void NCriticalPathView::onDataCleared()
{
    hideControls();
    m_inputFilter->clear();
    m_outputFilter->clear();
}

void NCriticalPathView::resizeEvent(QResizeEvent* event)
{
    updateControlsLocation();
    QTreeView::resizeEvent(event);
}

void NCriticalPathView::showEvent(QShowEvent* event)
{
    updateControlsLocation();
    QTreeView::showEvent(event);
}

bool NCriticalPathView::updateChildrenSelectionFor(NCriticalPathItem* pathItem, bool selected) const
{
    bool selectionChanged = false;

    if (!pathItem) {
        return selectionChanged;
    }

    if (!m_sourceModel) {
        return selectionChanged;
    }

    QItemSelectionModel* selectModel = selectionModel();
    if (!selectModel) {
        return selectionChanged;
    }

    // collect range of selectionIndexes for path elemenets to be selected or deselected
    QModelIndex topLeft; // init invalid
    QModelIndex bottomRight; // init invalid
    for (int i=0; i<pathItem->childCount(); ++i) {
        NCriticalPathItem* child = pathItem->child(i);
        if (child->isSelectable()) {
            for (int column=0; column<NCriticalPathItem::Column::END; column++) {
                QModelIndex sourceIndex = m_sourceModel->findPathElementIndex(pathItem, child->data(NCriticalPathItem::Column::DATA).toString(), column);
                if (sourceIndex.isValid()) {
                    QModelIndex selectIndex = m_filterModel->mapFromSource(sourceIndex);
                    if (selectIndex.isValid()) {
                        bool isSelected = selectModel->isSelected(selectIndex);
                        if (isSelected != selected) {
                            selectionChanged = true;
                        }
                        if (!topLeft.isValid()) {
                            topLeft = selectIndex;
                        }
                        bottomRight = selectIndex;
                    }
                }
            }
        }
    }

    // apply selection range to selection model
    if (selectionChanged && topLeft.isValid() && bottomRight.isValid()) {
        QItemSelection selectionItem(topLeft, bottomRight);
        selectModel->select(selectionItem, selected ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);       
    }

    return selectionChanged;
}

QString NCriticalPathView::getSelectedPathElements() const
{
    QItemSelectionModel* selectModel = selectionModel();

    std::map<std::size_t, std::set<std::size_t>> data;
    if (selectModel) {
        QModelIndexList selectedIndexes = selectModel->selectedIndexes();
        for (const QModelIndex& index: qAsConst(selectedIndexes)) {
            if (index.column() == 0) {
                QModelIndex sourceIndex = m_filterModel->mapToSource(index);
                if (sourceIndex.isValid()) {
                    NCriticalPathItem* item = static_cast<NCriticalPathItem*>(sourceIndex.internalPointer());
                    if (item) {
                        QString type{item->type()};
                        int elementIndex{item->id()};
                        int pathIndex{item->pathIndex()};
                        
                        if (pathIndex != -1) {
                            data[pathIndex].insert(elementIndex);
                        } 
                    }
                }
            }
        }
    }

    QString result;
    for (const auto& [key, values]: data) {
        result += QString::number(key) + "#";
        for (int v: values) {
            result += QString::number(v) + ",";
        }
        if (result.endsWith(",")) {
            result.chop(1); // remove last coma 
        }
        result += "|";       
    }
    if (result.endsWith("|")) {
        result.chop(1); // remove last semicolomn 
    }
    if (result.isEmpty()) {
        result = "null"; // we cannot send just empty, because it breaks option parser
    }
    return result;
}

void NCriticalPathView::updateControlsLocation()
{
    const int offset = NCriticalPathTheme::instance().viewFloatingItemsOffset();
    int verticalScrollBarWidth = 0;
    if (verticalScrollBar() && verticalScrollBar()->isVisible()) {
        verticalScrollBarWidth = verticalScrollBar()->width();
    }
    m_bnFilter->move(size().width() - m_bnFilter->width() - verticalScrollBarWidth - offset, offset); // -15 here is to exclude the size of vertical scroll bar
    m_bnExpandCollapse->move(offset, offset);
    m_bnClearSelection->move(offset, offset + offset + m_bnExpandCollapse->height());
}
