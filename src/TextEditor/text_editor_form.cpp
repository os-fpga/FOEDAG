#include "text_editor_form.h"

#include <QVBoxLayout>

TextEditorForm::TextEditorForm(QWidget *parent) :
    QWidget(parent)
{

}

TextEditorForm::~TextEditorForm()
{

}

void TextEditorForm::InitForm()
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setSpacing(0);
    vbox->setContentsMargins(0, 0, 0, 0);
    m_tabWidget = new QTabWidget(this);
    vbox->addWidget(m_tabWidget);
    setLayout(vbox);
}
