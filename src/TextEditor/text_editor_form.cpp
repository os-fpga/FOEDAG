#include "text_editor_form.h"

#include <QMessageBox>
#include <QPainter>
#include <QStyleOption>

using namespace FOEDAG;

TextEditorForm *TextEditorForm::Instance() {
  static auto textEditorForm = new TextEditorForm;
  return textEditorForm;
}

void TextEditorForm::InitForm() {
  static bool initForm;
  if (initForm) {
    return;
  }

  m_tab_editor = new TabWidget(this);
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

#ifndef USE_MONACO_EDITOR
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
#endif  // #ifndef USE_MONACO_EDITOR

  initForm = true;

  connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this,
          &TextEditorForm::fileModifiedOnDisk);
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

  int filetype{FILE_TYPE_UNKOWN};
  QFileInfo fileInfo(strFileName);
  if (!fileInfo.exists()) return -1;
  QString filename = fileInfo.fileName();
  QString suffix = fileInfo.suffix();
  static const std::map<FileType, QStringList> types{
      {FILE_TYPE_CPP, {"c", "cpp", "cxx", "h", "hxx"}},
      {FILE_TYPE_VERILOG, {"v", "sv", "svi", "svh"}},
      {FILE_TYPE_VHDL, {"vhd"}},
      {FILE_TYPE_TCL, {"tcl"}},
  };
  auto typeIter =
      std::find_if(types.cbegin(), types.cend(),
                   [&suffix](const std::pair<FileType, QStringList> &pair) {
                     for (const auto &ext : pair.second) {
                       if (suffix.compare(ext, Qt::CaseInsensitive) == 0)
                         return true;
                     }
                     return false;
                   });
  if (typeIter != types.cend()) filetype = typeIter->first;

  FOEDAG::Editor *editor = new FOEDAG::Editor(strFileName, filetype, this);
  connect(editor, SIGNAL(EditorModificationChanged(bool)), this,
          SLOT(SlotUpdateTabTitle(bool)));
  connect(editor, &Editor::EditorModificationChanged, this, [=](bool m) {
    // file saved
    if (m == false) emit FileChanged(strFileName);
  });
#ifndef USE_MONACO_EDITOR
  connect(editor, SIGNAL(ShowSearchDialog(QString)), this,
          SLOT(SlotShowSearchDialog(QString)));
#endif  // #ifndef USE_MONACO_EDITOR

  index = m_tab_editor->addTab(editor, filename);
  m_tab_editor->setCurrentIndex(index);
  m_tab_editor->setTabToolTip(index, fileInfo.absoluteFilePath());

  QPair<int, Editor *> pair;
  pair.first = index;
  pair.second = editor;
  m_map_file_tabIndex_editor.insert(strFileName, pair);
  m_fileWatcher.addPath(strFileName);
  editor->SetFileWatcher(&m_fileWatcher);

  return editor->fileLoaded() ? 0 : -1;
}

int TextEditorForm::OpenFileWithLine(const QString &strFileName, int line,
                                     bool error) {
  int res = OpenFile(strFileName);
  if (res == 0) {
    if (line != -1) {
      auto pair = m_map_file_tabIndex_editor.value(strFileName);
      pair.second->clearMarkers();
      if (error)
        pair.second->markLineError(line);
      else
        pair.second->markLineWarning(line);
    }
  } else
    return -1;
  return 0;
}

int TextEditorForm::OpenFileWithSelection(const QString &strFileName,
                                          int lineFrom, int lineTo) {
  int res = OpenFile(strFileName);
  if (res == 0) {
    auto pair = m_map_file_tabIndex_editor.value(strFileName);
    pair.second->clearMarkers();
    pair.second->selectLines(lineFrom, lineTo);
  } else
    return -1;
  return 0;
}

void TextEditorForm::SlotTabCloseRequested(int index) {
  TabCloseRequested(index);
}

void TextEditorForm::SlotCurrentChanged(int index) {
  Editor *tabEditor = qobject_cast<Editor *>(m_tab_editor->widget(index));
  emit CurrentFileChanged(tabEditor ? tabEditor->getFileName() : QString{});
}

void TextEditorForm::SlotUpdateTabTitle(bool m) {
  int index = m_tab_editor->indexOf(qobject_cast<Editor *>(sender()));
  if (index != -1) {
    QString strName = m_tab_editor->tabText(index);
    if (m) {
      m_tab_editor->setTabText(index, strName + tr("*"));
    } else {
      m_tab_editor->setTabText(index, strName.left(strName.lastIndexOf("*")));
    }
  }
}

void TextEditorForm::SlotShowSearchDialog(const QString &strWord) {
#ifndef USE_MONACO_EDITOR
  m_searchDialog->InsertSearchWord(strWord);
  m_searchDialog->show();
#endif  // #ifndef USE_MONACO_EDITOR
}

void TextEditorForm::SlotFind(const QString &strFindWord) {
#ifndef USE_MONACO_EDITOR
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->FindFirst(strFindWord);
  }
#endif  // #ifndef USE_MONACO_EDITOR
}

void TextEditorForm::SlotFindNext(const QString &strFindWord) {
#ifndef USE_MONACO_EDITOR
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->FindNext(strFindWord);
  }
#endif  // #ifndef USE_MONACO_EDITOR
}

void TextEditorForm::SlotReplace(const QString &strFindWord,
                                 const QString &strDesWord) {
#ifndef USE_MONACO_EDITOR
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->Replace(strFindWord, strDesWord);
  }
#endif  // #ifndef USE_MONACO_EDITOR
}

void TextEditorForm::SlotReplaceAndFind(const QString &strFindWord,
                                        const QString &strDesWord) {
#ifndef USE_MONACO_EDITOR
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->ReplaceAndFind(strFindWord, strDesWord);
  }
#endif  // #ifndef USE_MONACO_EDITOR
}

void TextEditorForm::SlotReplaceAll(const QString &strFindWord,
                                    const QString &strDesWord) {
#ifndef USE_MONACO_EDITOR
  Editor *tabEditor = (Editor *)m_tab_editor->currentWidget();
  if (tabEditor) {
    tabEditor->ReplaceAll(strFindWord, strDesWord);
  }
#endif  // #ifndef USE_MONACO_EDITOR
}

void TextEditorForm::fileModifiedOnDisk(const QString &path) {
  auto editorPair = m_map_file_tabIndex_editor.value(path, {0, nullptr});
  auto editor{editorPair.second};
  if (editor) {
    // Here we need add path again since file descriptor might be closed by
    // other application and file watcher doesn't track this file anymore. If
    // path has already added - nothing happened.
    m_fileWatcher.addPath(path);
    if (editor->isModified()) {
      if (m_fileReloadDialogShown) {
        // file reload question dialog is already active
        // don't create more until user dismisses it.
        return;
      }
      m_fileReloadDialogShown = true;
      const QFileInfo info{path};
      auto res = QMessageBox::question(
          this, "File changed",
          QString{
              "The file %1 has been changed on disk. Do you want to reload it?"}
              .arg(info.fileName()));
      if (res == QMessageBox::No) {
        m_fileReloadDialogShown = false;
        return;
      }
    }
    editor->reload();
    m_fileReloadDialogShown = false;
  }
}

bool TextEditorForm::TabCloseRequested(int index) {
  if (index == -1) return false;

  Editor *tabItem = qobject_cast<Editor *>(m_tab_editor->widget(index));
  if (!tabItem) {
    m_tab_editor->removeTab(index);
    return true;
  }

#ifdef USE_MONACO_EDITOR
  bool willSaveAndClose = false;
#endif  // #ifdef USE_MONACO_EDITOR
  QString strName = m_tab_editor->tabText(index);
  QString strFileName = tabItem->getFileName();
  if (tabItem->isModified()) {
    int ret = QMessageBox::question(
        this, tr(""), tr("Save changes in %1?").arg(strName),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (ret == QMessageBox::Save) {
#ifdef USE_MONACO_EDITOR
      // ask Editor to save the file and delete itself:
      tabItem->SaveAndClose();
      willSaveAndClose = true;
#else   // #ifdef USE_MONACO_EDITOR
      tabItem->Save();
#endif  // #ifdef USE_MONACO_EDITOR
    } else if (ret == QMessageBox::Cancel) {
      return false;
    }
  }

  auto iter = m_map_file_tabIndex_editor.find(strFileName);
  if (iter != m_map_file_tabIndex_editor.end()) {
    m_map_file_tabIndex_editor.erase(iter);
    m_fileWatcher.removePath(iter.key());
  }
  // Removes the tab at position index from this stack of widgets.
  // The page widget itself is not deleted.
  m_tab_editor->removeTab(index);

#ifdef USE_MONACO_EDITOR
  if (!willSaveAndClose) {
    delete (tabItem);
  }
#else   // #ifdef USE_MONACO_EDITOR
  delete (tabItem);
#endif  // #ifdef USE_MONACO_EDITOR
  tabItem = nullptr;
  return true;
}

TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent) {}

void TabWidget::resizeEvent(QResizeEvent *event) {
  QTabWidget::resizeEvent(event);
  emit resized(event->size());
}
