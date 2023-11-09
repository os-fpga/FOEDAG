#include "ncriticalpathfilterwidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>

NCriticalPathFilterWidget::NCriticalPathFilterWidget(const QString& name, QWidget* parent)
    : QGroupBox(name, parent)
{
    QFormLayout* layout = new QFormLayout;
    setLayout(layout);

    m_comboBox = new QComboBox;
    m_lineEdit = new QLineEdit;
    m_chUseRegexp = new QCheckBox(tr("Use regular expressions"));
    m_chUseCaseSensetive = new QCheckBox(tr("Case sensitive"));
    QLabel* lbSearch = new QLabel("Search:");

//    m_lineEdit->setEnabled(false);
//    m_chUseRegexp->setEnabled(false);
//    m_chUseCaseSensetive->setEnabled(false);
//    lbSearch->setEnabled(false);

    layout->addRow(new QWidget, m_comboBox);
    layout->addRow(lbSearch, m_lineEdit);
    layout->addRow(new QWidget, wrapIntoRowWidget(m_chUseRegexp, m_chUseCaseSensetive));
}

void NCriticalPathFilterWidget::fillComboBox(const std::map<QString, int>& data)
{
    m_comboBox->clear();
    for (const auto& [name, counter]: data) {
        QString item = name;
        if (counter > 1) {
            item += QString(" (%1)").arg(counter);
        }
        m_comboBox->addItem(item);
    }
}
