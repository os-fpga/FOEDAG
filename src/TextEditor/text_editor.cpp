#include "text_editor.h"

#include "MainWindow/Session.h"
#include "text_editor_form.h"

using namespace FOEDAG;

void TextEditor::RegisterCommands(FOEDAG::Session* session) {
  registerTextEditorCommands(this, session);
}

void FOEDAG::registerTextEditorCommands(QWidget* editor,
                                        FOEDAG::Session* session) {
  auto texteditorshow = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::TextEditor* textEditor = (FOEDAG::TextEditor*)(clientData);
    textEditor->ShowTextEditor();
    return 0;
  };
  session->TclInterp()->registerCmd("texteditor_show", texteditorshow, editor,
                                    0);

  auto texteditorhide = [](void* clientData, Tcl_Interp* interp, int argc,
                           const char* argv[]) -> int {
    Q_UNUSED(interp);
    Q_UNUSED(argv);
    Q_UNUSED(argc);
    FOEDAG::TextEditor* textEditor = (FOEDAG::TextEditor*)(clientData);
    textEditor->ClosetextEditor();
    return 0;
  };
  session->TclInterp()->registerCmd("texteditor_close", texteditorhide, editor,
                                    0);

  auto openfile = [](void* clientData, Tcl_Interp* interp, int argc,
                     const char* argv[]) -> int {
    Tcl_ResetResult(interp);
    if (argc != 2) {
      const QString usageMsg = QString("Usage: %1 ?filename?").arg(argv[0]);
      Tcl_AppendResult(interp, qPrintable(usageMsg), (char*)nullptr);
      return TCL_ERROR;
    }
    auto editor = static_cast<FOEDAG::TextEditor*>(clientData);
    const QString file{argv[1]};
    const QFileInfo info{file};
    if (!info.exists()) {
      const QString msg = QString("File \"%1\" doesn't exists").arg(file);
      Tcl_AppendResult(interp, qPrintable(msg), (char*)nullptr);
      return TCL_ERROR;
    }
    auto res = editor->SlotOpenFile(file);
    return (res == 0) ? TCL_OK : TCL_ERROR;
  };
  session->TclInterp()->registerCmd(
      "openfile", openfile, static_cast<void*>(editor), nullptr /*deleteProc*/);
}

TextEditor::TextEditor(QWidget* parent) : QWidget(parent) {
  TextEditorForm::Instance()->InitForm();
  connect(TextEditorForm::Instance(), SIGNAL(CurrentFileChanged(QString)), this,
          SLOT(SlotCurrentFileChanged(QString)));
  connect(TextEditorForm::Instance(), &TextEditorForm::FileChanged, this,
          &TextEditor::FileChanged);
}

void TextEditor::ShowTextEditor() { TextEditorForm::Instance()->show(); }

void TextEditor::ClosetextEditor() { TextEditorForm::Instance()->hide(); }

QWidget* TextEditor::GetTextEditor() { return TextEditorForm::Instance(); }

int TextEditor::SlotOpenFile(const QString& strFileName) {
  return TextEditorForm::Instance()->OpenFile(strFileName);
}

void TextEditor::SlotOpenFileWithLine(const QString& strFileName, int line) {
  TextEditorForm::Instance()->OpenFileWithLine(strFileName, line);
}

void TextEditor::SlotCurrentFileChanged(const QString& strFileName) {
  emit CurrentFileChanged(strFileName);
}
