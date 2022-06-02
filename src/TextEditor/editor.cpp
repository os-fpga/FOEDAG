#include "editor.h"

using namespace FOEDAG;

#define ERROR_MARKER 4

Editor::Editor(QString strFileName, int iFileType, QWidget *parent)
    : QWidget(parent) {
  m_strFileName = strFileName;
  m_toolBar = new QToolBar(this);
  m_toolBar->setIconSize(QSize(32, 32));
  InitToolBar();

  m_scintilla = new QsciScintilla(this);
  InitScintilla(iFileType);
  SetScintillaText(strFileName);

  connect(m_scintilla, SIGNAL(textChanged()), this,
          SLOT(QScintillaTextChanged()));
  connect(m_scintilla, SIGNAL(selectionChanged()), this,
          SLOT(QscintillaSelectionChanged()));
  connect(m_scintilla, SIGNAL(modificationChanged(bool)), this,
          SLOT(QscintillaModificationChanged(bool)));

  QBoxLayout *box = new QBoxLayout(QBoxLayout::TopToBottom);
  box->setContentsMargins(0, 0, 0, 0);
  box->setSpacing(0);
  box->addWidget(m_toolBar);
  box->addWidget(m_scintilla);
  setLayout(box);
  QImage img(":/images/error.png");
  img = img.scaled(15, 15);
  m_scintilla->markerDefine(img, ERROR_MARKER);
}

QString Editor::getFileName() const { return m_strFileName; }

bool Editor::isModified() const { return m_scintilla->isModified(); }

void Editor::FindFirst(const QString &strWord) {
  m_scintilla->findFirst(strWord, true, true, true, true, false);
  m_scintilla->findNext();
}

void Editor::FindNext(const QString &strWord) {
  m_scintilla->findFirst(strWord, true, true, true, true);
}

void Editor::Replace(const QString &strFind, const QString &strDesWord) {
  Q_UNUSED(strFind);
  m_scintilla->replace(strDesWord);
}

void Editor::ReplaceAndFind(const QString &strFind, const QString &strDesWord) {
  m_scintilla->replace(strDesWord);
  m_scintilla->findFirst(strFind, true, true, true, true);
}

void Editor::ReplaceAll(const QString &strFind, const QString &strDesWord) {
  while (m_scintilla->findFirst(strFind, true, true, true, true)) {
    m_scintilla->replace(strDesWord);
  }
}

void Editor::markLine(int line) {
  m_scintilla->markerAdd(line - 1, ERROR_MARKER);
}

void Editor::clearMarkers() { m_scintilla->markerDeleteAll(ERROR_MARKER); }

void Editor::Search() {
  QString strWord = "";
  if (m_scintilla->hasSelectedText()) {
    strWord = m_scintilla->selectedText();
  }
  emit ShowSearchDialog(strWord);
}

void Editor::Save() {
  QFile file(m_strFileName);
  if (!file.open(QFile::WriteOnly)) {
    return;
  }

  QTextStream out(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  out << m_scintilla->text();
  QApplication::restoreOverrideCursor();

  m_scintilla->setModified(false);
}

void Editor::Undo() { m_scintilla->undo(); }

void Editor::Redo() { m_scintilla->redo(); }

void Editor::Cut() { m_scintilla->cut(); }

void Editor::Copy() { m_scintilla->copy(); }

void Editor::Paste() { m_scintilla->paste(); }

void Editor::Delete() { m_scintilla->removeSelectedText(); }

void Editor::SelectAll() { m_scintilla->selectAll(); }

void Editor::QscintillaSelectionChanged() { UpdateToolBarStates(); }

void Editor::QscintillaModificationChanged(bool m) {
  m_actSave->setEnabled(m);
  emit EditorModificationChanged(m);
}

void Editor::QScintillaTextChanged() { UpdateToolBarStates(); }

void Editor::InitToolBar() {
  m_actSearch = new QAction(m_toolBar);
  // m_actSearch->setIcon( QIcon(""));
  m_actSearch->setText(tr("&Search"));
  m_actSearch->setShortcut(tr("Ctrl+F"));
  m_toolBar->addAction(m_actSearch);
  m_toolBar->addSeparator();

  m_actSave = new QAction(m_toolBar);
  // m_actSave->setIcon( QIcon(""));
  m_actSave->setText(tr("&Save"));
  m_actSave->setShortcut(tr("Ctrl+S"));
  m_actSave->setEnabled(false);
  m_toolBar->addAction(m_actSave);
  m_toolBar->addSeparator();

  m_actUndo = new QAction(m_toolBar);
  // m_actUndo->setIcon( QIcon(""));
  m_actUndo->setText(tr("&Undo"));
  m_actUndo->setShortcut(tr("Ctrl+Z"));
  m_actUndo->setEnabled(false);
  m_toolBar->addAction(m_actUndo);
  m_toolBar->addSeparator();

  m_actRedo = new QAction(m_toolBar);
  // m_actRedo->setIcon( QIcon(""));
  m_actRedo->setText(tr("&Redo"));
  m_actRedo->setShortcut(tr("Ctrl+Y"));
  m_actRedo->setEnabled(false);
  m_toolBar->addAction(m_actRedo);
  m_toolBar->addSeparator();

  m_actCut = new QAction(m_toolBar);
  // m_actCut->setIcon( QIcon(""));
  m_actCut->setText(tr("&Cut"));
  m_actCut->setShortcut(tr("Ctrl+X"));
  m_actCut->setEnabled(false);
  m_toolBar->addAction(m_actCut);
  m_toolBar->addSeparator();

  m_actCopy = new QAction(m_toolBar);
  // m_actCopy->setIcon( QIcon(""));
  m_actCopy->setText(tr("&Copy"));
  m_actCopy->setShortcut(tr("Ctrl+C"));
  m_actCopy->setEnabled(false);
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
  m_actDelete->setEnabled(false);
  m_toolBar->addAction(m_actDelete);
  m_toolBar->addSeparator();

  m_actSelect = new QAction(m_toolBar);
  // m_actSelect->setIcon( QIcon(""));
  m_actSelect->setText(tr("&Select"));
  m_actSelect->setShortcut(tr("Ctrl+A"));
  m_toolBar->addAction(m_actSelect);

  connect(m_actSearch, SIGNAL(triggered()), this, SLOT(Search()));
  connect(m_actSave, SIGNAL(triggered()), this, SLOT(Save()));
  connect(m_actUndo, SIGNAL(triggered()), this, SLOT(Undo()));
  connect(m_actRedo, SIGNAL(triggered()), this, SLOT(Redo()));
  connect(m_actCut, SIGNAL(triggered()), this, SLOT(Cut()));
  connect(m_actCopy, SIGNAL(triggered()), this, SLOT(Copy()));
  connect(m_actPaste, SIGNAL(triggered()), this, SLOT(Paste()));
  connect(m_actDelete, SIGNAL(triggered()), this, SLOT(Delete()));
  connect(m_actSelect, SIGNAL(triggered()), this, SLOT(SelectAll()));
}

void Editor::InitScintilla(int iFileType) {
  QFont font("Arial", 9, QFont::Normal);
  m_scintilla->setFont(font);
  QFontMetrics fontmetrics = QFontMetrics(font);
  m_scintilla->setMarginWidth(0, 27 /*fontmetrics.width("0000")*/);

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

  m_scintilla->setModified(false);
}

void Editor::UpdateToolBarStates() {
  m_actUndo->setEnabled(m_scintilla->isUndoAvailable());
  m_actRedo->setEnabled(m_scintilla->isRedoAvailable());

  m_actCut->setEnabled(m_scintilla->hasSelectedText());
  m_actCopy->setEnabled(m_scintilla->hasSelectedText());
  m_actDelete->setEnabled(m_scintilla->hasSelectedText());
}
