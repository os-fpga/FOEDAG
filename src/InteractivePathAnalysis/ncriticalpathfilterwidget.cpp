#include "ncriticalpathfilterwidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
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

    m_lineEdit->setPlaceholderText(tr("type criteria here..."));

    QLabel* lbSearch = new QLabel("Search:");
    QPushButton* bnClear = new QPushButton();
    bnClear->setIcon(QIcon(":/images/erase.png"));
    connect(bnClear, &QPushButton::clicked, this, [this](){        
        resetUI();
    });

    layout->addRow(lbSearch, wrapIntoRowWidget(wrapIntoRowWidget(m_lineEdit, m_comboBox), bnClear));
    layout->addRow(new QWidget, wrapIntoRowWidget(m_chUseRegexp, m_chUseCaseSensetive));

    connect(m_comboBox, &QComboBox::currentTextChanged, this, [this](QString text){
        if (text == KEY_ANY_MASK) {
            m_lineEdit->clear();
        } else {
            // remove occurance counter
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
        resetComboBoxSilently();
    });

    connect(m_chUseRegexp, &QCheckBox::stateChanged, this, [this](bool isChecked){
        m_chUseCaseSensetive->setEnabled(!isChecked);
    });
}

void NCriticalPathFilterWidget::onAccepted()
{
    backupUI();
}

void NCriticalPathFilterWidget::onDeclined()
{
    if (m_backup.isValid) {
        restoreUIFromBackup();
    } else {
        resetUI();
    }
}

void NCriticalPathFilterWidget::backupUI()
{
    m_backup.criteria = m_lineEdit->text();
    m_backup.comboBoxItemIndex = m_comboBox->currentIndex();
    m_backup.checkBoxCaseSensetive = m_chUseCaseSensetive->isChecked();
    m_backup.checkBoxRegexp = m_chUseRegexp->isChecked();

    m_backup.isValid = true;
}

void NCriticalPathFilterWidget::restoreUIFromBackup()
{
    if (m_backup.isValid) {
        m_lineEdit->setText(m_backup.criteria);
        m_comboBox->setCurrentIndex(m_backup.comboBoxItemIndex);
        m_chUseCaseSensetive->setChecked(m_backup.checkBoxCaseSensetive);
        m_chUseRegexp->setChecked(m_backup.checkBoxRegexp);
    }
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

void NCriticalPathFilterWidget::resetUI()
{
    m_comboBox->setCurrentIndex(0);
    m_lineEdit->clear();
    m_chUseRegexp->setChecked(false);
    m_chUseCaseSensetive->setChecked(false);
}

void NCriticalPathFilterWidget::resetComboBoxSilently()
{
    m_comboBox->blockSignals(true);
    m_comboBox->setCurrentIndex(0);
    m_comboBox->blockSignals(false);
}

void NCriticalPathFilterWidget::resetLineEditSilently()
{
    m_lineEdit->blockSignals(true);
    m_lineEdit->clear();
    m_lineEdit->blockSignals(false);
}
