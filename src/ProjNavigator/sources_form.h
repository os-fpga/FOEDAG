#ifndef SOURCES_FORM_H
#define SOURCES_FORM_H

#include <QWidget>

namespace Ui {
class SourcesForm;
}

class SourcesForm : public QWidget
{
    Q_OBJECT

public:
    explicit SourcesForm(QWidget *parent = nullptr);
    ~SourcesForm();

private:
    Ui::SourcesForm *ui;
};

#endif // SOURCES_FORM_H
