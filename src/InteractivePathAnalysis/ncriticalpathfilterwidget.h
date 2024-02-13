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

    struct UiBackup {
        bool isValid = false;

        int comboBoxItemIndex = -1;
        QString criteria;
        bool checkBoxCaseSensetive = false;
        bool checkBoxRegexp = false;

        void clear() {
            isValid = false;

            comboBoxItemIndex = -1;
            criteria = "";
            checkBoxCaseSensetive = false;
            checkBoxRegexp = false;
        }
    };

public:
    explicit NCriticalPathFilterWidget(const QString& name, QWidget* parent = nullptr);
    ~NCriticalPathFilterWidget()=default;

    void fillComboBoxWithNodes(const std::map<QString, int>& data);

    FilterCriteriaConf criteriaConf() const;

    QComboBox* comboBox() const { return m_comboBox; }

    void clear();

    void onAccepted();
    void onDeclined();

private:
    QComboBox* m_comboBox = nullptr;
    QLineEdit* m_lineEdit = nullptr;
    QCheckBox* m_chUseRegexp = nullptr;
    QCheckBox* m_chUseCaseSensetive = nullptr;

    UiBackup m_backup;    

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

    void backupUI();
    void restoreUIFromBackup();

    void resetUI();
};
