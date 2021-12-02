#include "search_dialog.h"

#include <QVBoxLayout>

SearchDialog::SearchDialog(QWidget *parent) : QDialog(parent) {
  m_labelFind = new QLabel(tr("Find:"), this);
  m_labelReplace = new QLabel(tr("Replace with:"), this);

  m_comboBoxFind = CreateComboBox();
  m_comboBoxReplace = CreateComboBox();

  m_btnFindPrevious =
      CreateButton(tr("&Find Previous"), SLOT(SlotFindPrevious()));
  m_btnFindNext = CreateButton(tr("&Find Next"), SLOT(SlotFindNext()));

  m_btnReplace = CreateButton(tr("&Replace"), SLOT(SlotReplace()));
  m_btnReplaceAndFind =
      CreateButton(tr("&Replace&&Find"), SLOT(SlotReplaceAndFind()));
  m_btnReplaceAll = CreateButton(tr("&Replace All"), SLOT(SlotReplaceAll()));

  QGridLayout *mainLayout = new QGridLayout(this);
  mainLayout->addWidget(m_labelFind, 0, 0);
  mainLayout->addWidget(m_comboBoxFind, 0, 1, 1, 2);
  mainLayout->addWidget(m_labelReplace, 1, 0);
  mainLayout->addWidget(m_comboBoxReplace, 1, 1, 1, 2);
  mainLayout->addWidget(m_btnFindPrevious, 0, 4);
  mainLayout->addWidget(m_btnFindNext, 0, 5);
  mainLayout->addWidget(m_btnReplace, 1, 4);
  mainLayout->addWidget(m_btnReplaceAndFind, 1, 5);
  mainLayout->addWidget(m_btnReplaceAll, 1, 6);
  setLayout(mainLayout);
  this->setWindowTitle(tr("Find"));
  resize(500, 80);
}

void SearchDialog::SlotFindPrevious() {}

void SearchDialog::SlotFindNext() {}

void SearchDialog::SlotReplace() {}

void SearchDialog::SlotReplaceAndFind() {}

void SearchDialog::SlotReplaceAll() {}

QPushButton *SearchDialog::CreateButton(const QString &text,
                                        const char *member) {
  QPushButton *button = new QPushButton(text, this);
  connect(button, SIGNAL(clicked()), this, member);
  return button;
}

QComboBox *SearchDialog::CreateComboBox(const QString &text) {
  QComboBox *comboBox = new QComboBox(this);
  comboBox->setEditable(true);
  comboBox->addItem(text);
  comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  return comboBox;
}
