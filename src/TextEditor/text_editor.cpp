#include "text_editor.h"

using namespace FOEDAG;

TextEditor::TextEditor(QWidget *parent) : QWidget(parent) {
  TextEditorForm::Instance()->InitForm();
  connect(TextEditorForm::Instance(), SIGNAL(CurrentFileChanged(QString)), this,
          SLOT(SlotCurrentFileChanged(QString)));
}

void TextEditor::ShowTextEditor() { TextEditorForm::Instance()->show(); }

void TextEditor::ClosetextEditor() { TextEditorForm::Instance()->hide(); }

QWidget *TextEditor::GetTextEditor() { return TextEditorForm::Instance(); }

void TextEditor::SlotOpenFile(const QString &strFileName) {
  TextEditorForm::Instance()->OpenFile(strFileName);
}

void TextEditor::SlotCurrentFileChanged(const QString &strFileName) {
  emit CurrentFileChanged(strFileName);
}
