#pragma once

#include <QTreeView>

class CustomMenu;
class NCriticalPathFilterWidget;
class FilterCriteriaConf;

class QPushButton;
class QCheckBox;

class NCriticalPathView final: public QTreeView
{
    Q_OBJECT

public:
    explicit NCriticalPathView(QWidget* parent = nullptr);
    ~NCriticalPathView() override final = default;

    void fillInputOutputData(const std::map<QString, int>&, const std::map<QString, int>&);
    void select(const QString& pathId);

    void setModel(QAbstractItemModel* model) override final;

protected:
    void resizeEvent(QResizeEvent*) override final;
    void showEvent(QShowEvent*) override final;

    void handleSelection();

signals:
    void pathSelectionChanged(const QString&, const QString&);
    void criteriaFilterChanged(const FilterCriteriaConf&, const FilterCriteriaConf&);

public slots:
    void onDataLoaded();
    void onDataCleared();

private slots:
#ifdef ENABLE_SELECTION_RESTORATION
    void refreshSelection();
#endif

private:
    QString m_lastSelectedPathId;
    QPushButton* m_bnExpandCollapse = nullptr;
    bool m_isCollapsed = true;

    QPushButton* m_bnFilter = nullptr;
    CustomMenu* m_filterMenu = nullptr;

    NCriticalPathFilterWidget* m_inputFilter = nullptr;
    NCriticalPathFilterWidget* m_outputFilter = nullptr;

    void setupFilterMenu();

    void updateControlsLocation();
    void hideControls();

    bool isItemIndexVisible(const QModelIndex&) const;

#ifdef ENABLE_MULTISELECTION_MODE
    QList<QString> getSelectedItems() const;
#endif
    QModelIndex getSelectedIndex() const;
};

