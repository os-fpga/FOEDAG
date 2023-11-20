#include "ncriticalpathfilterwidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QRegularExpression>

NCriticalPathFilterWidget::NCriticalPathFilterWidget(const QString& name, QWidget* parent)
    : QGroupBox(name, parent)
    , m_comboBox(new QComboBox)
    , m_lineEdit(new QLineEdit)
    , m_chUseRegexp(new QCheckBox(tr("Use regular expressions")))
    , m_chUseCaseSensetive(new QCheckBox(tr("Case sensitive")))
{
    QFormLayout* layout = new QFormLayout;
    setLayout(layout);

    QLabel* lbSearch = new QLabel("Search:");

    //layout->addRow(new QWidget, m_comboBox);
    layout->addRow(lbSearch, wrapIntoRowWidget(m_lineEdit, m_comboBox));
    layout->addRow(new QWidget, wrapIntoRowWidget(m_chUseRegexp, m_chUseCaseSensetive));

    connect(m_comboBox, &QComboBox::currentTextChanged, this, [this](QString text){
        if (text == KEY_ANY_MASK) {
            m_lineEdit->clear();
        } else {
            // remove occuranceCounter
            static QRegularExpression occuranceCounterPattern(" \\((\\d+)\\)");
            if (QRegularExpressionMatch match = occuranceCounterPattern.match(text); match.hasMatch()) {
                QString occuranceCounterStr = match.captured(0);
                text.chop(occuranceCounterStr.size());
            }

            m_lineEdit->blockSignals(true);
            m_lineEdit->setText(text);
            m_lineEdit->blockSignals(false);

        }
    });

    connect(m_lineEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        m_comboBox->blockSignals(true);
        m_comboBox->setCurrentIndex(0);
        m_comboBox->blockSignals(false);
    });

#ifndef ENABLE_FILTER_REGEXP
    m_chUseRegexp->setEnabled(false);
#endif
}

void NCriticalPathFilterWidget::fillComboBoxWithNodes(const std::map<QString, int>& data)
{
    m_comboBox->clear();
    m_comboBox->addItem(KEY_ANY_MASK);
    for (const auto& [name, counter]: data) {
        QString item = name;
        if (counter > 1) {
            item += QString(" (%1)").arg(counter);
        }
        m_comboBox->addItem(item);
    }
}

FilterCriteriaConf NCriticalPathFilterWidget::criteriaConf() const
{
    return FilterCriteriaConf{m_lineEdit->text(), m_chUseCaseSensetive->isChecked(), m_chUseRegexp->isChecked()};
}
