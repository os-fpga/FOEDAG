#pragma once

#include <QTreeView>

class CustomMenu;
class NCriticalPathFilterWidget;

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
    QList<QString> getSelectedItems() const;

protected:
    void resizeEvent(QResizeEvent*) override final;
    void showEvent(QShowEvent*) override final;

signals:
    void pathSelectionChanged(const QString&, const QString&);

public slots:
    void refreshSelection();
    void onDataLoaded();
    void onDataCleared();

private:
    QString m_lastSelectedPathId;
    QPushButton* m_bnExpandCollapse = nullptr;
    bool m_isCollapsed = true;

    QPushButton* m_bnFilter = nullptr;
    CustomMenu* m_filterMenu = nullptr;
    QCheckBox* m_cbSaveSettings = nullptr;

    NCriticalPathFilterWidget* m_inputFilter = nullptr;
    NCriticalPathFilterWidget* m_outputFilter = nullptr;

    void setupFilterMenu();

    void updateControlsLocation();
    void hideControls();
};

