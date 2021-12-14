#ifndef CREATE_RUNS_DIALOG_H
#define CREATE_RUNS_DIALOG_H

#include <QDialog>

namespace Ui {
class CreateRunsDialog;
}

class CreateRunsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateRunsDialog(QWidget *parent = nullptr);
    ~CreateRunsDialog();

private:
    Ui::CreateRunsDialog *ui;
};

#endif // CREATE_RUNS_DIALOG_H
