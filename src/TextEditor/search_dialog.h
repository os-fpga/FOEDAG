#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QObject>
#include <QPushButton>

class SearchDialog : public QDialog {
  Q_OBJECT
 public:
  explicit SearchDialog(QWidget *parent = nullptr);

 signals:

 private slots:
  void SlotFindPrevious();
  void SlotFindNext();
  void SlotReplace();
  void SlotReplaceAndFind();
  void SlotReplaceAll();

 private:
  QLabel *m_labelFind;
  QLabel *m_labelReplace;
  QComboBox *m_comboBoxFind;
  QComboBox *m_comboBoxReplace;
  QPushButton *m_btnFindPrevious;
  QPushButton *m_btnFindNext;
  QPushButton *m_btnReplace;
  QPushButton *m_btnReplaceAndFind;
  QPushButton *m_btnReplaceAll;

  QPushButton *CreateButton(const QString &text, const char *member);
  QComboBox *CreateComboBox(const QString &text = QString());
};

#endif  // SEARCHDIALOG_H
