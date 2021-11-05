#ifndef LOCATIONFORM_H
#define LOCATIONFORM_H

#include <QWidget>

namespace Ui {
class locationForm;
}

class locationForm : public QWidget
{
    Q_OBJECT

public:
    explicit locationForm(QWidget *parent = nullptr);
    ~locationForm();

    QString getprojectname();
    QString getprojectpath();
    bool iscreatedir();
    bool isprojectNameExit();
private slots:
    void on_m_btnBrowse_clicked();

    void on_m_checkBox_stateChanged(int arg1);

    void on_m_lineEditPname_textChanged(const QString &arg1);

private:
    Ui::locationForm *ui;
};

#endif // LOCATIONFORM_H
