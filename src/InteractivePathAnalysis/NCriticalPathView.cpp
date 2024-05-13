#include "NCriticalPathView.h"
#include "NCriticalPathModel.h"
#include "NCriticalPathItemDelegate.h"
#include "NCriticalPathFilterWidget.h"
#include "NCriticalPathFilterModel.h"
#include "NCriticalPathModel.h"
#include "NCriticalPathTheme.h"
#include "NCriticalPathParameters.h"
#include "NCriticalPathItem.h"
#include "RoundProgressWidget.h"

#include "CustomMenu.h"
#include "SimpleLogger.h"
#include "client/CommConstants.h"

#include <QScrollBar>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QMouseEvent>
#include <QKeyEvent>

#include <map>
#include <set>

#define IPA_DISABLE_CTRL_A_KEYSEQUENCE

NCriticalPathView::NCriticalPathView(QWidget* parent)
    : QTreeView(parent)
    , m_sourceModel(new NCriticalPathModel(this))
    , m_filterModel(new NCriticalPathFilterModel(this))
{
    setupModels();

    setSelectionMode(QAbstractItemView::MultiSelection);

    setExpandsOnDoubleClick(false); // will be redirected on single mouse right button press
    setVerticalScrollMode(QTreeView::ScrollPerPixel);
    verticalScrollBar()->setSingleStep(m_scrollStep);

    setPalette(NCriticalPathTheme::instance().selectedItemPallete());

    NCriticalPathItemDelegate* customDelegate = new NCriticalPathItemDelegate(this);
    setItemDelegate(customDelegate);

    const int iconSize = NCriticalPathTheme::instance().iconSize();

    // setup expand controls
    m_bnExpandCollapse = new QPushButton(this);
    m_bnExpandCollapse->setToolTip(tr("expand/collapse critical path items"));
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
    m_bnClearSelection->setToolTip(tr("clear selected critical paths and selected critical path elements"));
    m_bnClearSelection->setFixedSize(iconSize, iconSize);
    m_bnClearSelection->setIcon(QIcon(":/cross.png"));
    QObject::connect(m_bnClearSelection, &QPushButton::clicked, this, &NCriticalPathView::clearSelection);

    setupFilterMenu();

    m_overlay = new RoundProgressWidget(64, this);

    hideControls();
}

void NCriticalPathView::setupModels()
{
    m_filterModel->setSourceModel(m_sourceModel);
    setModel(m_filterModel);
    connect(m_sourceModel, &NCriticalPathModel::cleared, this, [this](){
        onActualDataCleared();
        emit dataCleared();
    });
    connect(m_sourceModel, &NCriticalPathModel::loadFinished, this, [this](){
        onActualDataLoaded();
        emit dataLoaded();
    });

    connect(this, &NCriticalPathView::loadFromString, m_sourceModel, &NCriticalPathModel::loadFromString);

    // selectionModel() is null before we set the model, that's why we create the connection after model set
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &NCriticalPathView::handleSelectionChanged);
}

void NCriticalPathView::clear()
{
    m_sourceModel->clear();
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

void NCriticalPathView::mouseReleaseEvent(QMouseEvent* event)
{
    while (!m_pathSourceIndexesToResolveChildrenSelection.empty()) {
        const auto& [sourceIndex, selected] = m_pathSourceIndexesToResolveChildrenSelection.takeLast();
        if (sourceIndex.isValid()) {
            updateChildrenSelectionFor(sourceIndex, selected);
        }
    }

    QTreeView::mouseReleaseEvent(event);
}

void NCriticalPathView::keyPressEvent(QKeyEvent* event)
{
#ifdef IPA_DISABLE_CTRL_A_KEYSEQUENCE
    if(event->matches(QKeySequence::SelectAll)) {
        event->ignore();
        return;
    }
#endif

    switch (event->key()) {
        case Qt::Key_Up: {
            scroll(-m_scrollStep);
            break;
        }
        case Qt::Key_Down: {
            scroll(m_scrollStep);
            break;
        }
        default: {
            QTreeView::keyPressEvent(event);
        }
    }
}

void NCriticalPathView::scroll(int steps)
{
    verticalScrollBar()->setValue(verticalScrollBar()->value() + steps);
}

void NCriticalPathView::handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (!m_sourceModel) {
        return;
    }

    // we skip path children item selection resolving if clearAllSeletion event is pending
    if (!m_isClearAllSelectionsPending) {
        for (const QModelIndex& index: selected.indexes()) {
            if (index.isValid()) {
                QModelIndex sourceIndex = m_filterModel->mapToSource(index);
                    if (sourceIndex.column() == 0) {
                    NCriticalPathItem* item = static_cast<NCriticalPathItem*>(sourceIndex.internalPointer());
                    if (item) {
                        if (item->isPath()) {
                            m_pathSourceIndexesToResolveChildrenSelection.push_back(std::make_pair(sourceIndex, true));
                        }
                    }
                }
            }
        }

        for (const QModelIndex& index: deselected.indexes()) {
            if (index.isValid()) {
                QModelIndex sourceIndex = m_filterModel->mapToSource(index);
                if (sourceIndex.column() == 0) {
                    NCriticalPathItem* item = static_cast<NCriticalPathItem*>(sourceIndex.internalPointer());
                    if (item) {
                        if (item->isPath()) {
                            m_pathSourceIndexesToResolveChildrenSelection.push_back(std::make_pair(sourceIndex, false));
                        }
                    }
                }
            }
        }
    }

    if (m_pathSourceIndexesToResolveChildrenSelection.empty()) {
        QString selectedPathElements = getSelectedPathElements();
        m_bnClearSelection->setVisible(!selectedPathElements.isEmpty() && (selectedPathElements != comm::CRITICAL_PATH_ITEMS_SELECTION_NONE));
        emit pathElementSelectionChanged(selectedPathElements, "selectedPathElements");
        //qInfo() << "selectedPathElements=" << selectedPathElements;
        SimpleLogger::instance().log("selectedPathElements:", selectedPathElements);
    }

    if (m_isClearAllSelectionsPending) { // reset flag on next selection event
        m_isClearAllSelectionsPending = false;
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
    m_bnFilter->setToolTip(tr("configure filter criteria that will affect which critical path items will be visible"));
    int iconSize = NCriticalPathTheme::instance().iconSize();
    m_bnFilter->setFixedSize(iconSize,iconSize);
    m_bnFilter->setIcon(QIcon(":/search.png"));
    m_filterMenu = new CustomMenu(m_bnFilter);
    m_filterMenu->setButtonToolTips(
        tr("apply filter criteria"),
        tr("discard filter criteria")
    );
    m_filterMenu->setAlignment(CustomMenu::Alignment::RIGHT);

    QVBoxLayout* layout = new QVBoxLayout;
    m_filterMenu->addContentLayout(layout);

    m_inputFilter = new NCriticalPathFilterWidget(tr("Input Nodes:"));
    m_outputFilter = new NCriticalPathFilterWidget(tr("Output Nodes:"));

    connect(m_filterMenu, &CustomMenu::accepted, this, [this](){
        m_inputFilter->onAccepted();
        m_outputFilter->onAccepted();
        if (m_filterModel->setFilterCriteria(m_inputFilter->criteriaConf(), m_outputFilter->criteriaConf())) {
            clearSelection();
            handleSelectionChanged(QItemSelection{}, QItemSelection{}); // in some reason this slot is not called after we clearSelection when we modify filter model criteria, so we call it manually here
        }
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

void NCriticalPathView::onActualDataLoaded()
{
    fillInputOutputData(m_sourceModel->inputNodes(), m_sourceModel->outputNodes());
    m_bnExpandCollapse->setVisible(true);
    m_bnFilter->setVisible(true);
    hideBusyOverlay();
}

void NCriticalPathView::onActualDataCleared()
{
    hideControls();
    m_inputFilter->clear();
    m_outputFilter->clear();
    m_filterModel->clear();
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

void NCriticalPathView::updateChildrenSelectionFor(const QModelIndex& sourcePathIndex, bool selected) const
{
    if (!m_sourceModel) {
        return;
    }

    QItemSelectionModel* selectModel = selectionModel();
    if (!selectModel) {
        return;
    }

    NCriticalPathItem* pathItem = static_cast<NCriticalPathItem*>(sourcePathIndex.internalPointer());
    if (!pathItem) {
        return;
    }
    if (!pathItem->isPath()) {
        return;
    }

    // collect range of selectionIndexes for path elemenets to be selected or deselected

    QModelIndex sourceTopLeftIndex = m_sourceModel->index(0, 0, sourcePathIndex);
    QModelIndex sourceBottomRightIndex = m_sourceModel->index(pathItem->childCount()-1, pathItem->columnCount()-1, sourcePathIndex);

    QModelIndex selectTopLeftIndex = m_filterModel->mapFromSource(sourceTopLeftIndex);
    QModelIndex selectBottomRightIndex = m_filterModel->mapFromSource(sourceBottomRightIndex);

    // apply selection range to selection model
    if (selectTopLeftIndex.isValid() && selectBottomRightIndex.isValid()) {
        QItemSelection selectionItem(selectTopLeftIndex, selectBottomRightIndex);
        selectModel->select(selectionItem, selected ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);       
    }
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
                        //QString type{item->type()};
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
        result = comm::CRITICAL_PATH_ITEMS_SELECTION_NONE; // we cannot send just empty, because it breaks option parser
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
    m_overlay->move(0.5*size().width() - 0.5*m_overlay->width(), 0.5*size().height() - 0.5*m_overlay->height());
}

void NCriticalPathView::clearSelection()
{
    m_pathSourceIndexesToResolveChildrenSelection.clear();
    m_isClearAllSelectionsPending = true;

    QTreeView::clearSelection();
}

void NCriticalPathView::showBusyOverlay()
{
    m_overlay->show();
}

void NCriticalPathView::hideBusyOverlay()
{
    m_overlay->hide();
}
