#pragma once

#include <QGroupBox>
#include <QHBoxLayout>

#include "filtercriteriaconf.h"

class QComboBox;
class QLineEdit;
class QCheckBox;

class NCriticalPathFilterWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit NCriticalPathFilterWidget(const QString& name, QWidget* parent = nullptr);
    ~NCriticalPathFilterWidget()=default;

    void fillComboBoxWithNodes(const std::map<QString, int>& data);

    FilterCriteriaConf criteriaConf() const;

private:
    QComboBox* m_comboBox = nullptr;
    QLineEdit* m_lineEdit = nullptr;
    QCheckBox* m_chUseRegexp = nullptr;
    QCheckBox* m_chUseCaseSensetive = nullptr;

    template<typename T1, typename T2>
    QWidget* wrapIntoRowWidget(T1 w1, T2* w2) {
        QWidget* container = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout;
        container->setLayout(layout);
        layout->setContentsMargins(0,0,0,0);
        layout->addWidget(w1);
        layout->addWidget(w2);
        return container;
    }

    void resetComboBoxSilently();
    void resetLineEditSilently();
};
