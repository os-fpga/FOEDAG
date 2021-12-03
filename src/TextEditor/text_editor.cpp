#include "text_editor.h"

using namespace FOEDAG;

TextEditor::TextEditor(QWidget *parent) : QWidget(parent) {
  TextEditorForm::Instance()->InitForm();
}

void TextEditor::ShowTextEditor() { TextEditorForm::Instance()->show(); }

void TextEditor::ClosetextEditor() { TextEditorForm::Instance()->hide(); }

QWidget *TextEditor::GetTextEditor() { return TextEditorForm::Instance(); }

void TextEditor::OpenFile(const QString &strFileName) {
  TextEditorForm::Instance()->OpenFile(strFileName);
}
