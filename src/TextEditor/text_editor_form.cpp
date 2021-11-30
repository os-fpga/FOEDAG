#include "text_editor_form.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QVBoxLayout>

using namespace FOEDAG;

Q_GLOBAL_STATIC(TextEditorForm, texteditor)

TextEditorForm *TextEditorForm::Instance() { return texteditor(); }

void TextEditorForm::InitForm() {
  static bool initForm;
  if (initForm) {
    return;
  }

  m_tab_editor = new QTabWidget(this);
  m_tab_editor->setTabsClosable(true);

  if (this->layout() != nullptr) {
    delete this->layout();
  }
  QVBoxLayout *vbox = new QVBoxLayout(this);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(m_tab_editor);
  setLayout(vbox);

  initForm = true;
}

int TextEditorForm::OpenFile(const QString &strFileName) {
  int ret = 0;
  QFile file(strFileName);
  if (!file.open(QFile::ReadOnly)) {
    return -1;
  }

  int index = 0;
  auto iter = m_map_file_tabIndex_editor.find(strFileName);
  if (iter != m_map_file_tabIndex_editor.end()) {
    index = iter.value().first;
    m_tab_editor->setCurrentIndex(index);
    return ret;
  }

  int filetype;
  QFileInfo fileInfo(strFileName);
  QString filename = fileInfo.fileName();
  QString suffix = fileInfo.suffix();
  if (suffix.compare(QString("v"), Qt::CaseInsensitive) == 0) {
    filetype = FILE_TYPE_VERILOG;
  } else if (suffix.compare(QString("vhd"), Qt::CaseInsensitive) == 0) {
    filetype = FILE_TYPE_VHDL;
  } else if (suffix.compare(QString("tcl"), Qt::CaseInsensitive) == 0) {
    filetype = FILE_TYPE_TCL;
  } else {
    filetype = FILE_TYPE_UNKOWN;
  }

  QsciScintilla *qscintilla = CreateScintilla(filetype);
  if (nullptr == qscintilla) {
    return -1;
  }

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  qscintilla->setText(in.readAll());
  QApplication::restoreOverrideCursor();
  qscintilla->setObjectName(strFileName);

  index = m_tab_editor->addTab(qscintilla, filename);
  m_tab_editor->setCurrentIndex(index);

  QPair<int, QsciScintilla *> pair;
  pair.first = index;
  pair.second = qscintilla;
  m_map_file_tabIndex_editor.insert(strFileName, pair);

  return ret;
}

QsciScintilla *TextEditorForm::CreateScintilla(int iFileType) {
  QsciScintilla *qscintilla = new QsciScintilla(this);
  if (nullptr == qscintilla) {
    return nullptr;
  }
  qscintilla->setMarginType(0, QsciScintilla::NumberMargin);
  qscintilla->setMarginLineNumbers(0, true);
  qscintilla->setTabWidth(4);
  qscintilla->setAutoIndent(true);
  qscintilla->setIndentationGuides(QsciScintilla::SC_IV_LOOKBOTH);
  qscintilla->setBraceMatching(QsciScintilla::SloppyBraceMatch);

  QsciLexer *textLexer;
  if (FILE_TYPE_VERILOG == iFileType) {
    textLexer = new QsciLexerVerilog(qscintilla);
  } else if (FILE_TYPE_VHDL == iFileType) {
    textLexer = new QsciLexerVHDL(qscintilla);
  } else if (FILE_TYPE_TCL == iFileType) {
    textLexer = new QsciLexerTCL(qscintilla);
  }

  if (FILE_TYPE_VERILOG == iFileType || FILE_TYPE_VHDL == iFileType ||
      FILE_TYPE_TCL == iFileType) {
    qscintilla->setLexer(textLexer);

    QsciAPIs *apis = new QsciAPIs(textLexer);
    apis->add(QString("begin"));
    apis->add(QString("always"));
    apis->prepare();

    qscintilla->setAutoCompletionSource(QsciScintilla::AcsAll);
    qscintilla->setAutoCompletionCaseSensitivity(true);
    qscintilla->setAutoCompletionThreshold(1);
  }

  qscintilla->setCaretLineVisible(true);
  qscintilla->setCaretLineBackgroundColor(Qt::lightGray);
  qscintilla->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,
                            QsciScintilla::SC_CP_UTF8);
  qscintilla->setFolding(QsciScintilla::BoxedTreeFoldStyle);
  qscintilla->setFoldMarginColors(Qt::gray, Qt::lightGray);

  // connect(qscintilla, SIGNAL(textChanged()), this,
  // SLOT(documentWasModified()));

  return qscintilla;
}
