#include "search_dialog.h"

#include <QVBoxLayout>

using namespace FOEDAG;

SearchDialog::SearchDialog(QWidget *parent) : QDialog(parent) {
  m_labelFind = new QLabel(tr("Find:"), this);
  m_labelReplace = new QLabel(tr("Replace with:"), this);

  m_editFind = new QLineEdit(this);

  m_editReplace = new QLineEdit(this);

  m_btnFindPrevious = new QPushButton(tr("&Find Previous"), this);
  connect(m_btnFindPrevious, SIGNAL(clicked()), this, SLOT(SlotFindPrevious()));

  m_btnFindNext = new QPushButton(tr("&Find Next"), this);
  connect(m_btnFindNext, SIGNAL(clicked()), this, SLOT(SlotFindNext()));

  m_btnReplace = new QPushButton(tr("&Replace"), this);
  connect(m_btnReplace, SIGNAL(clicked()), this, SLOT(SlotReplace()));

  m_btnReplaceAndFind = new QPushButton(tr("&Replace&&Find"), this);
  connect(m_btnReplaceAndFind, SIGNAL(clicked()), this,
          SLOT(SlotReplaceAndFind()));

  m_btnReplaceAll = new QPushButton(tr("&Replace All"), this);
  connect(m_btnReplaceAll, SIGNAL(clicked()), this, SLOT(SlotReplaceAll()));

  QGridLayout *mainLayout = new QGridLayout(this);
  mainLayout->addWidget(m_labelFind, 0, 0);
  mainLayout->addWidget(m_editFind, 0, 1, 1, 2);
  mainLayout->addWidget(m_labelReplace, 1, 0);
  mainLayout->addWidget(m_editReplace, 1, 1, 1, 2);
  mainLayout->addWidget(m_btnFindPrevious, 0, 3);
  mainLayout->addWidget(m_btnFindNext, 0, 4);
  mainLayout->addWidget(m_btnReplace, 1, 3);
  mainLayout->addWidget(m_btnReplaceAndFind, 1, 4);
  mainLayout->addWidget(m_btnReplaceAll, 1, 5);
  setLayout(mainLayout);
  setWindowTitle(tr("Find"));
  resize(500, 80);
}

void SearchDialog::InsertSearchWord(const QString &strWord) {
  m_editFind->setText(strWord);
}

void SearchDialog::SlotFindPrevious() { emit Find(m_editFind->text()); }

void SearchDialog::SlotFindNext() { emit FindNext(); }

void SearchDialog::SlotReplace() {
  emit Replace(m_editFind->text(), m_editReplace->text());
}

void SearchDialog::SlotReplaceAndFind() {
  emit ReplaceAndFind(m_editFind->text(), m_editReplace->text());
}

void SearchDialog::SlotReplaceAll() {
  emit ReplaceAll(m_editFind->text(), m_editReplace->text());
}
