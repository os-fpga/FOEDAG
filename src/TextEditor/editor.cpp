#include "editor.h"

using namespace FOEDAG;

Editor::Editor(QString strFileName, int iFileType, QWidget *parent)
    : QWidget(parent) {
  m_scintilla = new QsciScintilla(this);
  InitScintilla(iFileType);
  SetScintillaText(strFileName);

  QBoxLayout *l = new QBoxLayout(QBoxLayout::TopToBottom);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(0);
  l->addWidget(m_scintilla);
  setLayout(l);
}

void Editor::InitScintilla(int iFileType) {
  m_scintilla->setMarginType(0, QsciScintilla::NumberMargin);
  m_scintilla->setMarginLineNumbers(0, true);
  m_scintilla->setTabWidth(4);
  m_scintilla->setAutoIndent(true);
  m_scintilla->setIndentationGuides(QsciScintilla::SC_IV_LOOKBOTH);
  m_scintilla->setBraceMatching(QsciScintilla::SloppyBraceMatch);

  QsciLexer *textLexer;
  if (FILE_TYPE_VERILOG == iFileType) {
    textLexer = new QsciLexerVerilog(m_scintilla);
  } else if (FILE_TYPE_VHDL == iFileType) {
    textLexer = new QsciLexerVHDL(m_scintilla);
  } else if (FILE_TYPE_TCL == iFileType) {
    textLexer = new QsciLexerTCL(m_scintilla);
  }

  if (FILE_TYPE_VERILOG == iFileType || FILE_TYPE_VHDL == iFileType ||
      FILE_TYPE_TCL == iFileType) {
    m_scintilla->setLexer(textLexer);

    QsciAPIs *apis = new QsciAPIs(textLexer);
    apis->add(QString("begin"));
    apis->add(QString("always"));
    apis->prepare();

    m_scintilla->setAutoCompletionSource(QsciScintilla::AcsAll);
    m_scintilla->setAutoCompletionCaseSensitivity(true);
    m_scintilla->setAutoCompletionThreshold(1);
  }

  m_scintilla->setCaretLineVisible(true);
  m_scintilla->setCaretLineBackgroundColor(Qt::lightGray);
  m_scintilla->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,
                             QsciScintilla::SC_CP_UTF8);
  m_scintilla->setFolding(QsciScintilla::BoxedTreeFoldStyle);
  m_scintilla->setFoldMarginColors(Qt::gray, Qt::lightGray);
}

void Editor::SetScintillaText(QString strFileName) {
  QFile file(strFileName);
  if (!file.open(QFile::ReadOnly)) {
    return;
  }

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  m_scintilla->setText(in.readAll());
  QApplication::restoreOverrideCursor();
}
