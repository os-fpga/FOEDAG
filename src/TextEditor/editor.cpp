#include "editor.h"

using namespace FOEDAG;

Editor::Editor(QString strFileName, int iFileType, QWidget *parent)
    : QWidget(parent) {
  m_strFileName = strFileName;
  m_toolBar = new QToolBar(this);
  m_toolBar->setIconSize(QSize(32, 32));
  InitToolBar();

  m_scintilla = new QsciScintilla(this);
  InitScintilla(iFileType);
  SetScintillaText(strFileName);
  //  connect(m_scintilla, SIGNAL(textChanged()),this,
  //  SLOT(documentWasModified())); connect(m_scintilla,
  //  SIGNAL(selectionChanged()),this, SLOT(documentWasModified()));

  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  box->addWidget(m_toolBar);
  box->addWidget(m_scintilla);
  setLayout(box);

  UpdateToolBarStates();
}

QString Editor::getFileName() const { return m_strFileName; }

bool Editor::isModified() const { return m_scintilla->isModified(); }

void Editor::Save() {
  QFile file(m_strFileName);
  if (!file.open(QFile::WriteOnly)) {
    return;
  }

  QTextStream out(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  out << m_scintilla->text();
  QApplication::restoreOverrideCursor();
}

void Editor::InitToolBar() {
  m_actSearch = new QAction(m_toolBar);
  // m_actSearch->setIcon( QIcon(""));
  m_actSearch->setText(tr("&Search"));
  m_actSearch->setShortcut(tr("Ctrl+R"));
  m_toolBar->addAction(m_actSearch);
  m_toolBar->addSeparator();

  m_actSave = new QAction(m_toolBar);
  // m_actSave->setIcon( QIcon(""));
  m_actSave->setText(tr("&Save"));
  m_actSave->setShortcut(tr("Ctrl+S"));
  m_toolBar->addAction(m_actSave);
  m_toolBar->addSeparator();

  m_actUndo = new QAction(m_toolBar);
  // m_actUndo->setIcon( QIcon(""));
  m_actUndo->setText(tr("&Undo"));
  m_actUndo->setShortcut(tr("Ctrl+Z"));
  m_toolBar->addAction(m_actUndo);
  m_toolBar->addSeparator();

  m_actRedo = new QAction(m_toolBar);
  // m_actRedo->setIcon( QIcon(""));
  m_actRedo->setText(tr("&Redo"));
  m_actRedo->setShortcut(tr("Ctrl+Y"));
  m_toolBar->addAction(m_actRedo);
  m_toolBar->addSeparator();

  m_actCut = new QAction(m_toolBar);
  // m_actCut->setIcon( QIcon(""));
  m_actCut->setText(tr("&Cut"));
  m_actCut->setShortcut(tr("Ctrl+X"));
  m_toolBar->addAction(m_actCut);
  m_toolBar->addSeparator();

  m_actCopy = new QAction(m_toolBar);
  // m_actCopy->setIcon( QIcon(""));
  m_actCopy->setText(tr("&Copy"));
  m_actCopy->setShortcut(tr("Ctrl+C"));
  m_toolBar->addAction(m_actCopy);
  m_toolBar->addSeparator();

  m_actPaste = new QAction(m_toolBar);
  // m_actPaste->setIcon( QIcon(""));
  m_actPaste->setText(tr("&Paste"));
  m_actPaste->setShortcut(tr("Ctrl+V"));
  m_toolBar->addAction(m_actPaste);
  m_toolBar->addSeparator();

  m_actDelete = new QAction(m_toolBar);
  // m_actDelete->setIcon( QIcon(""));
  m_actDelete->setText(tr("Delete"));
  m_toolBar->addAction(m_actDelete);
  m_toolBar->addSeparator();

  m_actSelect = new QAction(m_toolBar);
  // m_actSelect->setIcon( QIcon(""));
  m_actSelect->setText(tr("&Select"));
  m_actSelect->setShortcut(tr("Ctrl+A"));
  m_toolBar->addAction(m_actSelect);
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

void Editor::UpdateToolBarStates() {
  m_actSave->setEnabled(!m_scintilla->isModified());
  m_actUndo->setEnabled(m_scintilla->isUndoAvailable());
  m_actRedo->setEnabled(m_scintilla->isRedoAvailable());

  m_actCut->setEnabled(m_scintilla->hasSelectedText());
  m_actCopy->setEnabled(m_scintilla->hasSelectedText());
  m_actDelete->setEnabled(m_scintilla->hasSelectedText());
  //  QsciScintillaBase qscintillaBase;
  //  m_actPaste->setEnabled(
  //      qscintillaBase.SendScintilla(QsciScintillaBase::SCI_CANPASTE));
}
