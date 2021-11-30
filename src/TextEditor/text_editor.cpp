#include "text_editor.h"

using namespace FOEDAG;

TextEditor::TextEditor(QObject *parent) : QObject(parent) {
  TextEditorForm::Instance()->InitForm();
}

void TextEditor::ShowTextEditor() { TextEditorForm::Instance()->show(); }

QWidget *TextEditor::GetTextEditor() { return TextEditorForm::Instance(); }

void TextEditor::OpenFile(const QString &strFileName) {
  TextEditorForm::Instance()->OpenFile(strFileName);
}
