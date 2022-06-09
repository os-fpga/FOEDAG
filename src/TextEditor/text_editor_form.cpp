#include "text_editor_form.h"

#include <QMessageBox>
#include <QPainter>
#include <QStyleOption>

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
  connect(m_tab_editor, SIGNAL(tabCloseRequested(int)), this,
          SLOT(SlotTabCloseRequested(int)));
  connect(m_tab_editor, SIGNAL(currentChanged(int)), this,
          SLOT(SlotCurrentChanged(int)));

  if (this->layout() != nullptr) {
    delete this->layout();
  }
  QVBoxLayout *vbox = new QVBoxLayout(this);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(m_tab_editor);
  setLayout(vbox);

  m_searchDialog = new SearchDialog(this);
  connect(m_searchDialog, SIGNAL(Find(QString)), this, SLOT(SlotFind(QString)));
  connect(m_searchDialog, SIGNAL(FindNext(QString)), this,
          SLOT(SlotFindNext(QString)));

  connect(m_searchDialog, SIGNAL(Replace(QString, QString)), this,
          SLOT(SlotReplace(QString, QString)));
  connect(m_searchDialog, SIGNAL(ReplaceAndFind(QString, QString)), this,
          SLOT(SlotReplaceAndFind(QString, QString)));
  connect(m_searchDialog, SIGNAL(ReplaceAll(QString, QString)), this,
          SLOT(SlotReplaceAll(QString, QString)));

  initForm = true;
}

int TextEditorForm::OpenFile(const QString &strFileName) {
  int ret = 0;

  int index = 0;
  auto iter = m_map_file_tabIndex_editor.find(strFileName);
  if (iter != m_map_file_tabIndex_editor.end()) {
    index = iter.value().first;
    m_tab_editor->setCurrentIndex(index);
    return ret;
  }

  int filetype;
  QFileInfo fileInfo(strFileName);
  if (!fileInfo.exists()) return -1;
  QString filename = fileInfo.fileName();
  QString suffix = fileInfo.suffix();
  if (suffix.compare(QString("v"), Qt::CaseInsensitive) == 0 ||
      suffix.compare(QString("sv"), Qt::CaseInsensitive) == 0 ||
      suffix.compare(QString("svi"), Qt::CaseInsensitive) == 0 ||
      suffix.compare(QString("svh"), Qt::CaseInsensitive) == 0) {
    filetype = FILE_TYPE_VERILOG;
  } else if (suffix.compare(QString("vhd"), Qt::CaseInsensitive) == 0) {
    filetype = FILE_TYPE_VHDL;
  } else if (suffix.compare(QString("tcl"), Qt::CaseInsensitive) == 0) {
    filetype = FILE_TYPE_TCL;
  } else {
    filetype = FILE_TYPE_UNKOWN;
  }

  FOEDAG::Editor *editor = new FOEDAG::Editor(strFileName, filetype, this);
  connect(editor, SIGNAL(EditorModificationChanged(bool)), this,
          SLOT(SlotUpdateTabTitle(bool)));
  connect(editor, SIGNAL(ShowSearchDialog(QString)), this,
          SLOT(SlotShowSearchDialog(QString)));

  index = m_tab_editor->addTab(editor, filename);
  m_tab_editor->setCurrentIndex(index);

  QPair<int, Editor *> pair;
  pair.first = index;
  pair.second = editor;
  m_map_file_tabIndex_editor.insert(strFileName, pair);

  return ret;
}

int TextEditorForm::OpenFileWithLine(const QString &strFileName, int line) {
  int res = OpenFile(strFileName);
  if (res == 0) {
    if (line != -1) {
      auto pair = m_map_file_tabIndex_editor.value(strFileName);
      pair.second->clearMarkers();
      pair.second->markLine(line);
    }
  } else
    return -1;
  return 0;
}

void TextEditorForm::SlotTabCloseRequested(int index) {
  if (index == -1) {
    return;
  }

  Editor *tabItem = (Editor *)m_tab_editor->widget(index);
  QString strName = m_tab_editor->tabText(index);
  if (tabItem->isModified()) {
    int ret = QMessageBox::question(
        this, tr(""), tr("Save changes in %1?").arg(strName), QMessageBox::Yes,
        QMessageBox::No, QMessageBox::Cancel);
    if (ret == QMessageBox::Yes) {
      tabItem->Save();
    } else if (ret == QMessageBox::Cancel) {
      return;
    }
  }

  auto iter = m_map_file_tabIndex_editor.find(tabItem->getFileName());
  if (iter != m_map_file_tabIndex_editor.end()) {
    m_map_file_tabIndex_editor.erase(iter);
  }
  // Removes the tab at position index from this stack of widgets.
  // The page widget itself is not deleted.
  m_tab_editor->removeTab(index);

  delete (tabItem);
  tabItem = nullptr;
}

void TextEditorForm::SlotCurrentChanged(int index) {
  Editor *tabEditor = (Editor *)m_tab_editor->widget(index);
  if (tabEditor) {
    emit CurrentFileChanged(tabEditor->getFileName());
  }
}

void TextEditorForm::SlotUpdateTabTitle(bool m) {
  int index = m_tab_editor->currentIndex();
  QString strName = m_tab_editor->tabText(index);
  if (m) {
    m_tab_editor->setTabText(index, strName + tr("*"));
  } else {
    m_tab_editor->setTabText(index, strName.left(strName.lastIndexOf("*")));
  }
}

void TextEditorForm::SlotShowSearchDialog(const QString &strWord) {
  m_searchDialog->InsertSearchWord(strWord);
  m_searchDialog->show();
}

void TextEditorForm::SlotFind(const QString &strFindWord) {
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->FindFirst(strFindWord);
  }
}

void TextEditorForm::SlotFindNext(const QString &strFindWord) {
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->FindNext(strFindWord);
  }
}

void TextEditorForm::SlotReplace(const QString &strFindWord,
                                 const QString &strDesWord) {
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->Replace(strFindWord, strDesWord);
  }
}

void TextEditorForm::SlotReplaceAndFind(const QString &strFindWord,
                                        const QString &strDesWord) {
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->ReplaceAndFind(strFindWord, strDesWord);
  }
}

void TextEditorForm::SlotReplaceAll(const QString &strFindWord,
                                    const QString &strDesWord) {
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->ReplaceAll(strFindWord, strDesWord);
  }
}
