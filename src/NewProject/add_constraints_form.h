#ifndef ADDCONSTRAINTSFORM_H
#define ADDCONSTRAINTSFORM_H

#include <QWidget>
#include "source_grid.h"

namespace Ui {
class addConstraintsForm;
}

class addConstraintsForm : public QWidget
{
    Q_OBJECT

public:
    explicit addConstraintsForm(QWidget *parent = nullptr);
    ~addConstraintsForm();

    QList<filedata> getfiledata();
    bool iscopysource();
private:
    Ui::addConstraintsForm *ui;

    sourceGrid* m_widgetgrid;
};

#endif // ADDCONSTRAINTSFORM_H
