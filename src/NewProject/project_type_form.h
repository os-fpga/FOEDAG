#ifndef PROJECTTYPEFORM_H
#define PROJECTTYPEFORM_H

#include <QWidget>

enum project_type
{
    TYPE_RTL,
    TYPE_POST
};

namespace Ui {
class projectTypeForm;
}

class projectTypeForm : public QWidget
{
    Q_OBJECT

public:
    explicit projectTypeForm(QWidget *parent = nullptr);
    ~projectTypeForm();

    int getprojecttype();
    bool isaddsource();
private slots:
    void on_m_radioBtnRTL_clicked();

    void on_m_radioBtnPost_clicked();

private:
    Ui::projectTypeForm *ui;

    void updatestate();
};

#endif // PROJECTTYPEFORM_H
