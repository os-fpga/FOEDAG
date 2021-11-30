#ifndef TEXT_EDITOR_FORM_H
#define TEXT_EDITOR_FORM_H

#include <QTabWidget>
#include <QWidget>

class TextEditorForm : public QWidget
{
    Q_OBJECT

public:
    explicit TextEditorForm(QWidget *parent = nullptr);
    ~TextEditorForm();
    void InitForm();
private:
    QTabWidget *m_tabWidget;
};

#endif // TEXT_EDITOR_FORM_H
