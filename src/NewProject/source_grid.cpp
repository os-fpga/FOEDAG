#include "source_grid.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QStandardItem>
#include <QVBoxLayout>

#include "create_file_dialog.h"

sourceGrid::sourceGrid(GridType type, QWidget *parent) : QWidget(parent) {
  m_type = type;

  m_lisFileData.clear();
  //    m_toolBar = new QToolBar(this);
  //    m_toolBar->setIconSize(QSize(32,32));

  //    m_toolBtnAdd = new QToolButton(this);
  //    m_toolBtnAdd->setIcon(QIcon(":/img/img/add.png"));
  //    m_toolBtnAdd->setToolButtonStyle( Qt::ToolButtonIconOnly);
  //    m_toolBtnAdd->setPopupMode(QToolButton::InstantPopup);

  //    QMenu* tmenu = new QMenu(this);
  //    QAction* tactAddFile = new QAction(tr("Add Files"),this);
  //    connect(tactAddFile,&QAction::triggered,this,&sourceGrid::slot_addfiles);
  //    tmenu->addAction(tactAddFile);

  //    if(GT_CONSTRAINTS != m_type)
  //    {
  //        QAction* tactAddDir = new QAction(tr("Add Directories"),this);
  //        connect(tactAddDir,&QAction::triggered,this,&sourceGrid::slot_adddirectories);
  //        tmenu->addAction(tactAddDir);
  //    }

  //    if(GT_NETLIST != m_type)
  //    {
  //        QAction* tactCreate = new QAction(tr("Create File"),this);
  //        connect(tactCreate,&QAction::triggered,this,&sourceGrid::slot_createfile);
  //        tmenu->addAction(tactCreate);
  //    }

  /*    tmenu->setStyleSheet("QMenu{padding:6px 16px;
  spacing:5;font-size:20px;}\
  //                          QMenu::item{padding:6px 16px;font-size:20px;}\
  // QMenu::item:selected{background-color:rgb(204,222,253);}");
       m_toolBtnAdd->setMenu(tmenu);*/

  //    m_toolBar->addWidget(m_toolBtnAdd);
  //    m_toolBar->addSeparator();

  //    m_actDel = new QAction(this);
  //    m_actDel->setIcon( QIcon(":/img/img/del.png"));
  //    m_actDel->setEnabled(false);
  //    connect(m_actDel,&QAction::triggered,this,&sourceGrid::slot_deletegriditem);
  //    m_toolBar->addAction(m_actDel);
  //    m_toolBar->addSeparator();

  //    m_actUp = new QAction(this);
  //    m_actUp->setIcon(QIcon(":/img/img/up.png"));
  //    m_actUp->setEnabled(false);
  //    connect(m_actUp,&QAction::triggered,this,&sourceGrid::slot_upgriditem);
  //    m_toolBar->addAction(m_actUp);
  //    m_toolBar->addSeparator();

  //    m_actDown = new QAction(this);
  //    m_actDown->setIcon(QIcon(":/img/img/down.png"));
  //    m_actDown->setEnabled(false);
  //    connect(m_actDown,&QAction::triggered,this,&sourceGrid::slot_downgriditem);
  //    m_toolBar->addAction(m_actDown);
  m_btnAddFile = new QPushButton(tr("AddFile"), this);
  connect(m_btnAddFile, &QPushButton::clicked, this, &sourceGrid::AddFiles);
  m_btnAddDri = new QPushButton(tr("AddDir"), this);
  connect(m_btnAddDri, &QPushButton::clicked, this,
          &sourceGrid::AddDirectories);
  m_btnCreateFile = new QPushButton(tr("CreateFile"), this);
  connect(m_btnCreateFile, &QPushButton::clicked, this,
          &sourceGrid::CreateFile);
  m_btnDelete = new QPushButton(tr("Delete"), this);
  m_btnDelete->setEnabled(false);
  connect(m_btnDelete, &QPushButton::clicked, this,
          &sourceGrid::DeleteTableItem);
  m_btnMoveUp = new QPushButton(tr("MoveUp"), this);
  m_btnMoveUp->setEnabled(false);
  connect(m_btnMoveUp, &QPushButton::clicked, this, &sourceGrid::UpTableItem);
  m_btnMoveDown = new QPushButton(tr("MoveDown"), this);
  m_btnMoveDown->setEnabled(false);
  connect(m_btnMoveDown, &QPushButton::clicked, this,
          &sourceGrid::DownTableItem);

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(m_btnAddFile);
  hbox->addWidget(m_btnAddDri);
  if (GT_CONSTRAINTS == m_type) {
    m_btnAddDri->setVisible(false);
  }
  hbox->addWidget(m_btnCreateFile);
  hbox->addWidget(m_btnDelete);
  hbox->addWidget(m_btnMoveUp);
  hbox->addWidget(m_btnMoveDown);
  hbox->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding));

  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(1);

  m_tableViewSrc = new QTableView(this);

  // Set properties
  m_tableViewSrc->verticalHeader()->hide();
  m_tableViewSrc->verticalHeader()->setDefaultSectionSize(30);
  m_tableViewSrc->horizontalHeader()->setStretchLastSection(
      true);  // Last column adaptive width
  m_tableViewSrc->setEditTriggers(QTableView::NoEditTriggers);
  m_tableViewSrc->setSelectionBehavior(QTableView::SelectRows);
  m_tableViewSrc->setSelectionMode(
      QTableView::SingleSelection);  // Single line selection
  m_tableViewSrc->setAlternatingRowColors(
      true);  // Color separation between lines

  m_tableViewSrc->setStyleSheet(
      "QTableView {border: 1px solid rgb(230,230,230);}\
                          QTableView::item:selected{color:black;background: #63B8FF;}");
  m_tableViewSrc->setColumnWidth(0, 80);

  m_model = new QStandardItemModel();
  m_selectModel = new QItemSelectionModel(m_model);

  m_tableViewSrc->horizontalHeader()->setMinimumHeight(30);

  m_model->setHorizontalHeaderItem(0, new QStandardItem(tr("Index")));
  m_model->setHorizontalHeaderItem(1, new QStandardItem(tr("Name")));
  m_model->setHorizontalHeaderItem(2, new QStandardItem(tr("Location")));

  m_tableViewSrc->setModel(m_model);
  m_tableViewSrc->setSelectionModel(m_selectModel);
  connect(m_selectModel, &QItemSelectionModel::selectionChanged, this,
          &sourceGrid::TableViewSelectionChanged);

  QVBoxLayout *vbox = new QVBoxLayout();
  // vbox->addWidget(m_toolBar);
  vbox->addLayout(hbox);
  vbox->addWidget(m_tableViewSrc);

  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  setLayout(vbox);
}

QList<filedata> sourceGrid::getTableViewData() { return m_lisFileData; }

void sourceGrid::AddFiles() {
  QString fileformat;
  if (GT_SOURCE == m_type) {
    fileformat = QString(tr("Design Files(*.v *.vhd)"));
  } else if (GT_CONSTRAINTS == m_type) {
    fileformat = QString(tr("Design Files(*.SDC *.sdc)"));
  }
  QStringList fileNames =
      QFileDialog::getOpenFileNames(this, tr("Select File"), "", fileformat);
  foreach (QString str, fileNames) {
    QString name = str.right(str.size() - (str.lastIndexOf("/") + 1));
    QString path = str.left(str.lastIndexOf("/"));
    QString suffix = name.right(name.size() - (name.lastIndexOf(".") + 1));

    filedata fdata;
    fdata.m_isFolder = false;
    fdata.m_fileType = suffix;
    fdata.m_fileName = name;
    fdata.m_filePath = path;
    AddTableItem(fdata);
  }
}

void sourceGrid::AddDirectories() {
  QString pathName = QFileDialog::getExistingDirectory(
      this, tr("Select Directory"), "",
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if ("" == pathName) return;
  QString folder =
      pathName.right(pathName.size() - (pathName.lastIndexOf("/") + 1));
  QString path = pathName.left(pathName.lastIndexOf("/"));

  filedata fdata;
  fdata.m_isFolder = true;
  fdata.m_fileName = folder;
  fdata.m_filePath = path;
  AddTableItem(fdata);
}

void sourceGrid::CreateFile() {
  createFileDialog *createdlg = new createFileDialog(this);
  createdlg->initialDialog(m_type);
  connect(createdlg, &createFileDialog::sig_updateGrid, this,
          &sourceGrid::AddTableItem);
  createdlg->exec();
  disconnect(createdlg, &createFileDialog::sig_updateGrid, this,
             &sourceGrid::AddTableItem);
}

void sourceGrid::DeleteTableItem() {
  int curRow = m_selectModel->currentIndex().row();
  if (curRow < 0) return;
  QString strtemp = m_model->item(curRow, 1)->text();
  for (int i = 0; i < m_lisFileData.count(); ++i) {
    if (m_lisFileData[i].m_fileName == strtemp) {
      m_lisFileData.removeAt(i);
      break;
    }
  }
  m_model->removeRow(curRow);

  QStandardItemModel *model =
      qobject_cast<QStandardItemModel *>(m_tableViewSrc->model());
  if (model == nullptr) {
    return;
  }
  int column = m_model->columnCount();
  int rows = m_model->rowCount();
  if (curRow == rows && rows != 0) curRow--;
  m_tableViewSrc->setCurrentIndex(model->index(curRow, column));
  m_tableViewSrc->selectRow(curRow);

  if (0 == rows) {
    m_btnDelete->setEnabled(false);
  }
}

void sourceGrid::UpTableItem() {
  int curRow = m_selectModel->currentIndex().row();
  if (curRow < 0) return;
  MoveTableRow(curRow, curRow - 1);
}

void sourceGrid::DownTableItem() {
  int curRow = m_selectModel->currentIndex().row();
  if (curRow < 0) return;
  MoveTableRow(curRow, curRow + 1);
}

void sourceGrid::TableViewSelectionChanged() {
  int curRow = m_selectModel->currentIndex().row();
  if (curRow < 0) return;

  int rows = m_model->rowCount();
  m_btnDelete->setEnabled(true);

  if (rows > curRow + 1) {
    m_btnMoveDown->setEnabled(true);
  } else {
    m_btnMoveDown->setEnabled(false);
  }

  if (curRow > 0) {
    m_btnMoveUp->setEnabled(true);
  } else {
    m_btnMoveUp->setEnabled(false);
  }
  return;
}

void sourceGrid::AddTableItem(filedata fdata) {
  if (IsFileDataExit(fdata)) return;

  int rows = m_model->rowCount();
  QList<QStandardItem *> items;
  QStandardItem *item = nullptr;

  item = new QStandardItem(QString("%1").arg(rows + 1));
  item->setTextAlignment(Qt::AlignCenter);
  items.append(item);

  QIcon icon;
  if (0 == fdata.m_isFolder) {
    icon.addFile(":/img/file.png");
  } else {
    icon.addFile(":/img/folder.png");
  }
  item = new QStandardItem();
  item->setIcon(icon);
  item->setText(fdata.m_fileName);
  item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  items.append(item);

  item = new QStandardItem(fdata.m_filePath);
  item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  items.append(item);

  m_model->insertRow(rows, items);
  TableViewSelectionChanged();
  m_lisFileData.append(fdata);
}

void sourceGrid::MoveTableRow(int from, int to) {
  if (from == to) {
    return;
  }

  QStandardItemModel *model =
      qobject_cast<QStandardItemModel *>(m_tableViewSrc->model());
  if (model == nullptr) {
    return;
  }

  QList<QStandardItem *> listItem = model->takeRow(from);
  model->insertRow(to, listItem);

  int column = m_tableViewSrc->currentIndex().column();
  m_tableViewSrc->setCurrentIndex(model->index(to, column));
  m_tableViewSrc->selectRow(to);

  for (int i = 0; i < m_model->rowCount(); i++) {
    m_model->item(i, 0)->setText(QString::number(i + 1));
    m_model->item(i, 0)->setTextAlignment(Qt::AlignCenter);
  }
}

bool sourceGrid::IsFileDataExit(filedata fdata) {
  foreach (filedata fd, m_lisFileData) {
    if (fdata.m_fileName == fd.m_fileName) {
      return true;
    }
  }
  return false;
}
