/**
  * @file NCriticalPathFilterWidget.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NCriticalPathFilterWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QRegularExpression>

#include <vector>

namespace FOEDAG {

NCriticalPathFilterWidget::NCriticalPathFilterWidget(const QString& name, QWidget* parent)
    : QGroupBox(name, parent)
    , m_comboBox(new QComboBox)
    , m_lineEdit(new QLineEdit)
    , m_chUseRegexp(new QCheckBox(tr("Use regular expressions")))
    , m_chUseCaseSensetive(new QCheckBox(tr("Case sensitive")))
{
    m_chUseRegexp->setToolTip(tr("enable this option to interpret filter criteria as regular expression patterns"));
    m_chUseCaseSensetive->setToolTip(tr("enable case sensitivity for the filter criteria"));

    m_comboBox->setToolTip(tr("select node be used as filter criteria"));

    QFormLayout* layout = new QFormLayout;
    setLayout(layout);

    m_lineEdit->setPlaceholderText(tr("type criteria here..."));
    m_lineEdit->setToolTip(tr("set filter criteria here to affect on visible path items"));

    QLabel* lbSearch = new QLabel("Search:");
    QPushButton* bnClear = new QPushButton();
    bnClear->setToolTip(tr("clear filter criteria"));
    bnClear->setIcon(QIcon(":/images/erase.png"));
    connect(bnClear, &QPushButton::clicked, this, [this](){        
        resetUI();
    });

    layout->addRow(lbSearch, wrapIntoRowWidget(wrapIntoRowWidget(m_lineEdit, m_comboBox), bnClear));
    layout->addRow(new QWidget, wrapIntoRowWidget(m_chUseRegexp, m_chUseCaseSensetive));

    connect(m_comboBox, &QComboBox::currentTextChanged, this, [this](QString text){
        if (text == FilterCriteriaConf::KEY_ANY_MASK) {
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
    // sort element by the number of occurrence in the crit path list
    std::vector<std::pair<QString, int>> sortedData(data.begin(), data.end());
    std::sort(sortedData.begin(), sortedData.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    m_comboBox->clear();
    m_comboBox->addItem(FilterCriteriaConf::KEY_ANY_MASK);
    for (const auto& [nodeName, occurrence]: sortedData) {
        QString itemName{nodeName};
        if (occurrence > 1) {
            itemName += QString(" (%1)").arg(occurrence);
        }
        m_comboBox->addItem(itemName);
    }
}

FilterCriteriaConf NCriticalPathFilterWidget::criteriaConf() const
{
    return FilterCriteriaConf{m_lineEdit->text(), m_chUseCaseSensetive->isChecked(), m_chUseRegexp->isChecked()};
}

void NCriticalPathFilterWidget::clear()
{
    m_backup.clear();
    resetUI();
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

} // namespace FOEDAG