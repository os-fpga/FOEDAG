#ifndef ADDSOURCEFORM_H
#define ADDSOURCEFORM_H

#include <QWidget>
#include "source_grid.h"

namespace Ui {
class addSourceForm;
}

class addSourceForm : public QWidget
{
    Q_OBJECT

public:
    explicit addSourceForm(QWidget *parent = nullptr);
    ~addSourceForm();

    QList<filedata> getfiledata();
    bool iscopysource();
private:
    Ui::addSourceForm *ui;

    sourceGrid* m_widgetgrid;
};

#endif // ADDSOURCEFORM_H
