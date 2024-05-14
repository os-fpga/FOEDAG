#include "NCriticalPathFilterWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <vector>

NCriticalPathFilterWidget::NCriticalPathFilterWidget(const QString& name,
                                                     QWidget* parent)
    : QGroupBox(name, parent),
      m_comboBox(new QComboBox),
      m_lineEdit(new QLineEdit),
      m_chUseRegexp(new QCheckBox(tr("Use regular expressions"))),
      m_chUseCaseSensetive(new QCheckBox(tr("Case sensitive"))) {
  m_chUseRegexp->setToolTip(
      tr("enable this option to interpret filter criteria as regular "
         "expression patterns"));
  m_chUseCaseSensetive->setToolTip(
      tr("enable case sensitivity for the filter criteria"));

  m_comboBox->setToolTip(tr("select node be used as filter criteria"));

  QFormLayout* layout = new QFormLayout;
  setLayout(layout);

  m_lineEdit->setPlaceholderText(tr("type criteria here..."));
  m_lineEdit->setToolTip(
      tr("set filter criteria here to affect on visible path items"));

  QLabel* lbSearch = new QLabel("Search:");
  QPushButton* bnClear = new QPushButton();
  bnClear->setToolTip(tr("clear filter criteria"));
  bnClear->setIcon(QIcon(":/images/erase.png"));
  connect(bnClear, &QPushButton::clicked, this, [this]() { resetUI(); });

  layout->addRow(
      lbSearch,
      wrapIntoRowWidget(wrapIntoRowWidget(m_lineEdit, m_comboBox), bnClear));
  layout->addRow(new QWidget,
                 wrapIntoRowWidget(m_chUseRegexp, m_chUseCaseSensetive));

  connect(
      m_comboBox, &QComboBox::currentTextChanged, this, [this](QString text) {
        if (text == FilterCriteriaConf::KEY_ANY_MASK) {
          m_lineEdit->clear();
        } else {
          // remove occurance counter
          static QRegularExpression occuranceCounterPattern(" \\((\\d+)\\)");
          if (QRegularExpressionMatch match =
                  occuranceCounterPattern.match(text);
              match.hasMatch()) {
            QString occuranceCounterStr = match.captured(0);
            text.chop(occuranceCounterStr.size());
          }

          m_lineEdit->blockSignals(true);
          m_lineEdit->setText(text);
          m_lineEdit->blockSignals(false);
        }
      });

  connect(m_lineEdit, &QLineEdit::textChanged, this,
          [this](const QString&) { resetComboBoxSilently(); });

  connect(
      m_chUseRegexp, &QCheckBox::stateChanged, this,
      [this](bool isChecked) { m_chUseCaseSensetive->setEnabled(!isChecked); });
}

void NCriticalPathFilterWidget::onAccepted() { backupUI(); }

void NCriticalPathFilterWidget::onDeclined() {
  if (m_backup.isValid) {
    restoreUIFromBackup();
  } else {
    resetUI();
  }
}

void NCriticalPathFilterWidget::backupUI() {
  m_backup.criteria = m_lineEdit->text();
  m_backup.comboBoxItemIndex = m_comboBox->currentIndex();
  m_backup.checkBoxCaseSensetive = m_chUseCaseSensetive->isChecked();
  m_backup.checkBoxRegexp = m_chUseRegexp->isChecked();

  m_backup.isValid = true;
}

void NCriticalPathFilterWidget::restoreUIFromBackup() {
  if (m_backup.isValid) {
    m_lineEdit->setText(m_backup.criteria);
    m_comboBox->setCurrentIndex(m_backup.comboBoxItemIndex);
    m_chUseCaseSensetive->setChecked(m_backup.checkBoxCaseSensetive);
    m_chUseRegexp->setChecked(m_backup.checkBoxRegexp);
  }
}

void NCriticalPathFilterWidget::fillComboBoxWithNodes(
    const std::map<QString, int>& data) {
  // sort element by the number of occurrence in the crit path list
  std::vector<std::pair<QString, int>> sortedData(data.begin(), data.end());
  std::sort(sortedData.begin(), sortedData.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  m_comboBox->clear();
  m_comboBox->addItem(FilterCriteriaConf::KEY_ANY_MASK);
  for (const auto& [nodeName, occurrence] : sortedData) {
    QString itemName{nodeName};
    if (occurrence > 1) {
      itemName += QString(" (%1)").arg(occurrence);
    }
    m_comboBox->addItem(itemName);
  }
}

FilterCriteriaConf NCriticalPathFilterWidget::criteriaConf() const {
  return FilterCriteriaConf{m_lineEdit->text(),
                            m_chUseCaseSensetive->isChecked(),
                            m_chUseRegexp->isChecked()};
}

void NCriticalPathFilterWidget::clear() {
  m_backup.clear();
  resetUI();
}

void NCriticalPathFilterWidget::resetUI() {
  m_comboBox->setCurrentIndex(0);
  m_lineEdit->clear();
  m_chUseRegexp->setChecked(false);
  m_chUseCaseSensetive->setChecked(false);
}

void NCriticalPathFilterWidget::resetComboBoxSilently() {
  m_comboBox->blockSignals(true);
  m_comboBox->setCurrentIndex(0);
  m_comboBox->blockSignals(false);
}

void NCriticalPathFilterWidget::resetLineEditSilently() {
  m_lineEdit->blockSignals(true);
  m_lineEdit->clear();
  m_lineEdit->blockSignals(false);
}
