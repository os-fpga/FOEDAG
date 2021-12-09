#ifndef NEWFILEDIALOG_H
#define NEWFILEDIALOG_H
#include <QFileDialog>
#include <QObject>

namespace FOEDAG {
class NewFileDialog : public QFileDialog {
  Q_OBJECT
 public:
  NewFileDialog(QWidget *parent = nullptr);
  NewFileDialog(QWidget *parent = nullptr, const QString &caption = QString(),
                const QString &directory = QString(),
                const QString &filter = QString());

 protected:
  void closeEvent(QCloseEvent *event) override;
  void done(int result) override;
};
}  // namespace FOEDAG
#endif  // NEWFILEDIALOG_H
