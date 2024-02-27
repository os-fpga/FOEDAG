#pragma once

#include <QTreeView>

class CustomMenu;
class NCriticalPathFilterWidget;
class NCriticalPathItem;
class NCriticalPathModel;
class NCriticalPathFilterModel;
class FilterCriteriaConf;

class QPushButton;
class QCheckBox;
class QMouseEvent;

class NCriticalPathView final: public QTreeView
{
    Q_OBJECT

    const int INDENT_SIZE = 150;

public:
    explicit NCriticalPathView(QWidget* parent = nullptr);
    ~NCriticalPathView() override final = default;

    void clear();

protected:
    void resizeEvent(QResizeEvent*) override final;
    void showEvent(QShowEvent*) override final;
    void mousePressEvent(QMouseEvent* event) override final;
    void mouseReleaseEvent(QMouseEvent* event) override final;
    void keyPressEvent(QKeyEvent* event) override final;

    void handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

signals:
    void pathElementSelectionChanged(const QString&, const QString&);
    void loadFromString(const QString&);
    void dataLoaded();
    void dataCleared();

private:
    int m_scrollStep = 10;
    QList<std::pair<QModelIndex, bool>> m_pathSourceIndexesToResolveChildrenSelection;
    bool m_isClearAllSelectionsPending = false;

    QString m_lastSelectedPathId;
    QPushButton* m_bnExpandCollapse = nullptr;
    bool m_isCollapsed = true;
    QPushButton* m_bnClearSelection = nullptr;

    QPushButton* m_bnFilter = nullptr;
    CustomMenu* m_filterMenu = nullptr;

    NCriticalPathFilterWidget* m_inputFilter = nullptr;
    NCriticalPathFilterWidget* m_outputFilter = nullptr;

    NCriticalPathModel* m_sourceModel = nullptr;
    NCriticalPathFilterModel* m_filterModel = nullptr;

    void setupFilterMenu();

    void updateControlsLocation();
    void hideControls();

    QString getSelectedPathElements() const;
    void updateChildrenSelectionFor(const QModelIndex& sourcePathIndex, bool selected) const;
    void scroll(int steps);

    void clearSelection();
    void fillInputOutputData(const std::map<QString, int>&, const std::map<QString, int>&);

    void setupModels();
    void onActualDataLoaded();
    void onActualDataCleared();
};

