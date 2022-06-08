#include "text_editor_form.h"

#include <QMessageBox>
#include <QPainter>
#include <QStyleOption>

using namespace FOEDAG;

Q_GLOBAL_STATIC(TextEditorForm, texteditor)

TextEditorForm *TextEditorForm::Instance() { return texteditor(); }

class CloseButton : public QAbstractButton {
 public:
  explicit CloseButton(QWidget *parent = 0) : QAbstractButton(parent) {
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setToolTip(tr("Close Tab"));
  }
  QSize sizeHint() const override {
    ensurePolished();
    int width =
        style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, 0, this);
    int height =
        style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, 0, this);
    return QSize(width, height);
  }
  QSize minimumSizeHint() const override { return sizeHint(); }
  void enterEvent(QEvent *event) override {
    if (isEnabled()) update();
    QAbstractButton::enterEvent(event);
  }
  void leaveEvent(QEvent *event) override {
    if (isEnabled()) update();
    QAbstractButton::leaveEvent(event);
  }
  void paintEvent(QPaintEvent *event) override {
    QPainter p(this);
    QStyleOption opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;
    if (isEnabled() && underMouse() && !isChecked() && !isDown())
      opt.state |= QStyle::State_Raised;
    if (isChecked()) opt.state |= QStyle::State_On;
    if (isDown()) opt.state |= QStyle::State_Sunken;
    if (const QTabBar *tb = qobject_cast<const QTabBar *>(parent())) {
      int index = tb->currentIndex();
      QTabBar::ButtonPosition position =
          (QTabBar::ButtonPosition)style()->styleHint(
              QStyle::SH_TabBar_CloseButtonPosition, 0, tb);
      if (tb->tabButton(index, position) == this)
        opt.state |= QStyle::State_Selected;
    }
    QIcon::Mode mode =
        opt.state & QStyle::State_Enabled
            ? (opt.state & QStyle::State_Raised ? QIcon::Active : QIcon::Normal)
            : QIcon::Disabled;
    if (!(opt.state & QStyle::State_Raised) &&
        !(opt.state & QStyle::State_Sunken) &&
        !(opt.state & QStyle::State_Selected))
      mode = QIcon::Disabled;
    QIcon::State state =
        opt.state & QStyle::State_Sunken ? QIcon::On : QIcon::Off;

    if ((opt.state & QStyle::State_Enabled) &&
        (opt.state & QStyle::State_MouseOver))
      style()->proxy()->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &p,
                                      this);

    QIcon pixmap{":/img/closetab.png"};
    int size = style()->proxy()->pixelMetric(QStyle::PM_SmallIconSize);
    auto pix = pixmap.pixmap(nullptr, QSize(size, size), mode, state);
    style()->proxy()->drawItemPixmap(&p, opt.rect, Qt::AlignCenter, pix);
  }
};

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
  auto closeButton = new CloseButton(m_tab_editor);
  closeButton->resize(closeButton->sizeHint());
  m_tab_editor->tabBar()->setTabButton(index, QTabBar::RightSide, closeButton);
  connect(closeButton, &QAbstractButton::clicked, this, [this]() {
    QObject *object = sender();
    int tabToClose = -1;
    QTabBar::ButtonPosition closeSide = QTabBar::RightSide;
    for (int i = 0; i < m_tab_editor->count(); ++i) {
      if (m_tab_editor->tabBar()->tabButton(i, closeSide) == object) {
        tabToClose = i;
        break;
      }
    }
    if (tabToClose != -1)
      emit m_tab_editor->tabBar()->tabCloseRequested(tabToClose);
  });

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
