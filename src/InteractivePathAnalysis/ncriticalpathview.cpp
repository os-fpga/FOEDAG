#include "ncriticalpathview.h"
#include "ncriticalpathmodel.h"
#include "ncriticalpathitemdelegate.h"
#include "ncriticalpathfilterwidget.h"
#include "ncriticalpaththeme.h"
#include "ncriticalpathsettings.h"
#include "custommenu.h"

#include <QScrollBar>
#include <QPushButton>
#include <QCheckBox>
#include <QMouseEvent>
#include <QDebug>

NCriticalPathView::NCriticalPathView(QWidget* parent)
    : QTreeView(parent)
{
#ifdef ENABLE_MULTISELECTION_MODE
    setSelectionMode(QAbstractItemView::MultiSelection);
#endif

    setAutoScroll(false);

    setPalette(NCriticalPathTheme::instance().selectedItemPallete());

    NCriticalPathItemDelegate* customDelegate = new NCriticalPathItemDelegate(this);
    setItemDelegate(customDelegate);

    connect(this, &QAbstractItemView::clicked, this, [this](){
        QList<QString> selectedItems = getSelectedItems();
        if (!selectedItems.isEmpty()) {
            QString item = selectedItems.first();
            qInfo() << "selectedItem:" << item;
            emit pathSelectionChanged(item, "item selected");
        }
    });

    // setup expand controls
    m_bnExpandCollapse = new QPushButton(this);
    m_bnExpandCollapse->setFixedSize(24,24);
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

    setupFilterMenu();

    hideControls();
}

void NCriticalPathView::fillInputOutputData(const std::map<QString, int>& inputs, const std::map<QString, int>& outputs)
{
    m_inputFilter->fillComboBoxWithNodes(inputs);
    m_outputFilter->fillComboBoxWithNodes(outputs);
}

void NCriticalPathView::setupFilterMenu()
{
    if (m_filterMenu) {
        return;
    }
    m_bnFilter = new QPushButton(this);
    m_bnFilter->setFixedSize(24,24);
    m_bnFilter->setIcon(QIcon(":/search.png"));
    m_filterMenu = new CustomMenu(m_bnFilter);
    m_filterMenu->setAlignment(CustomMenu::Alignment::RIGHT);

    QVBoxLayout* layout = new QVBoxLayout;
    m_filterMenu->addContentLayout(layout);

    m_inputFilter = new NCriticalPathFilterWidget(tr("Input Nodes:"));
    m_outputFilter = new NCriticalPathFilterWidget(tr("Output Nodes:"));

    m_cbSaveSettings = new QCheckBox(tr("Save settings"));
    m_cbSaveSettings->setChecked(NCriticalPathSettings::instance().getSaveFilterSettings());
    connect(m_cbSaveSettings, &QCheckBox::toggled, this, [](bool checked){
        NCriticalPathSettings::instance().setSaveFilterSettings(checked);
    });

    layout->addWidget(m_inputFilter);
    layout->addWidget(m_outputFilter);
    layout->addWidget(m_cbSaveSettings);
}

void NCriticalPathView::hideControls()
{
    m_bnExpandCollapse->setIcon(QIcon(":/next.png"));
    m_bnExpandCollapse->setVisible(false);
    m_isCollapsed = true;

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

void NCriticalPathView::refreshSelection()
{
    if (!m_lastSelectedPathId.isEmpty()) {
        select(m_lastSelectedPathId);
    }
}

void NCriticalPathView::select(const QString& pathId)
{
    if (m_lastSelectedPathId != pathId) {
        m_lastSelectedPathId = pathId;
    }
    QItemSelectionModel* selection = selectionModel();
    if (selection) {
        QModelIndex selectedIndex = static_cast<NCriticalPathModel*>(model())->findPathIndex(pathId);
        if (selectedIndex.isValid()) {
            selection->select(selectedIndex, QItemSelectionModel::Select);
        }
    }
}

QList<QString> NCriticalPathView::getSelectedItems() const
{
    QList<QString> result;
    QItemSelectionModel* selection = selectionModel();

    if (selection) {
        QModelIndexList selectedIndexes = selection->selectedIndexes();
        for (const QModelIndex& index: qAsConst(selectedIndexes)) {
            QVariant data = index.data(Qt::DisplayRole); // Retrieve the data from the selected item
            result << data.toString();
        }
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
}
