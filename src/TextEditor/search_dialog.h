#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>

namespace FOEDAG {

class SearchDialog : public QDialog {
  Q_OBJECT
 public:
  explicit SearchDialog(QWidget *parent = nullptr);

  void InsertSearchWord(const QString &strWord);
 signals:
  void Find(QString);
  void FindNext();
  void Replace(QString, QString);
  void ReplaceAndFind(QString, QString);
  void ReplaceAll(QString, QString);

 private slots:
  void SlotFindPrevious();
  void SlotFindNext();
  void SlotReplace();
  void SlotReplaceAndFind();
  void SlotReplaceAll();

 private:
  QLabel *m_labelFind;
  QLabel *m_labelReplace;
  QLineEdit *m_editFind;
  QLineEdit *m_editReplace;
  QPushButton *m_btnFindPrevious;
  QPushButton *m_btnFindNext;
  QPushButton *m_btnReplace;
  QPushButton *m_btnReplaceAndFind;
  QPushButton *m_btnReplaceAll;
};
}  // namespace FOEDAG
#endif  // SEARCHDIALOG_H
