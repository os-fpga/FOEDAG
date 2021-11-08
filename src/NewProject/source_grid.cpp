#include "source_grid.h"
//#include "createfiledialog.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardItem>
#include <QVBoxLayout>

sourceGrid::sourceGrid(grid_type type, QWidget *parent) : QWidget(parent) {
  m_type = type;

  m_ftmap.insert("v", ST_V);
  m_ftmap.insert("vhd", ST_VHD);
  m_ftmap.insert("CDC", ST_CDC);
  m_ftmap.insert("vqm", ST_VQM);
  m_ftmap.insert("mif", ST_MIF);
  m_ftmap.insert("hex", ST_HEX);

  m_fdatalist.clear();
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
  QPushButton *btnAddFile = new QPushButton(tr("AddFile"), this);
  QPushButton *btnAddDri = new QPushButton(tr("AddDir"), this);
  QPushButton *btnCreateFile = new QPushButton(tr("CreateFile"), this);
  QPushButton *btnDelete = new QPushButton(tr("Delete"), this);
  QPushButton *btnMoveUp = new QPushButton(tr("MoveUp"), this);
  QPushButton *btnMoveDown = new QPushButton(tr("MoveDown"), this);

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(btnAddFile);
  hbox->addWidget(btnAddDri);
  if (GT_CONSTRAINTS == m_type) {
    btnAddDri->setVisible(false);
  }
  hbox->addWidget(btnCreateFile);
  hbox->addWidget(btnDelete);
  hbox->addWidget(btnMoveUp);
  hbox->addWidget(btnMoveDown);
  hbox->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding));

  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(1);

  m_grid = new QTableView(this);

  // Set properties
  m_grid->verticalHeader()->hide();
  m_grid->verticalHeader()->setDefaultSectionSize(30);
  m_grid->horizontalHeader()->setStretchLastSection(
      true);  // Last column adaptive width
  m_grid->setEditTriggers(QTableView::NoEditTriggers);
  m_grid->setSelectionBehavior(QTableView::SelectRows);
  m_grid->setSelectionMode(
      QTableView::SingleSelection);       // Single line selection
  m_grid->setAlternatingRowColors(true);  // Color separation between lines

  m_grid->setStyleSheet(
      "QTableView {border: 1px solid rgb(230,230,230);}\
                          QTableView::item:selected{color:black;background: #63B8FF;}");
  m_grid->setColumnWidth(0, 80);

  // add table header
  m_model = new QStandardItemModel();
  m_selectmodel = new QItemSelectionModel(m_model);

  m_grid->horizontalHeader()->setMinimumHeight(30);

  m_model->setHorizontalHeaderItem(0, new QStandardItem(tr("Index")));
  m_model->setHorizontalHeaderItem(1, new QStandardItem(tr("Name")));
  m_model->setHorizontalHeaderItem(2, new QStandardItem(tr("Location")));

  m_grid->setModel(m_model);
  m_grid->setSelectionModel(m_selectmodel);
  connect(m_selectmodel, &QItemSelectionModel::selectionChanged, this,
          &sourceGrid::slot_selectionChanged);

  QVBoxLayout *vbox = new QVBoxLayout();
  // vbox->addWidget(m_toolBar);
  vbox->addLayout(hbox);
  vbox->addWidget(m_grid);

  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  setLayout(vbox);
}

void sourceGrid::initsourcegrid(const QList<filedata> flist) {
  for (int cnt = 0; cnt < flist.count(); ++cnt) {
    filedata pfdt = flist[cnt];
    filedata fdata;
    fdata.isfolder = pfdt.isfolder;
    fdata.filetype = pfdt.filetype;
    fdata.strtype = pfdt.strtype;
    fdata.fname = pfdt.fname;
    fdata.fpath = pfdt.fpath;
    slot_addgridrow(fdata);
  }
}

QList<filedata> sourceGrid::getgriddata() { return m_fdatalist; }

void sourceGrid::slot_addfiles() {
  QString fileformat;
  if (GT_SOURCE == m_type) {
    fileformat = QString(tr("Design Files(*.v *.vhd)"));
  } else if (GT_CONSTRAINTS == m_type) {
    fileformat = QString(tr("Design Files(*.CDC)"));
  } else if (GT_NETLIST == m_type) {
    fileformat = QString(tr("Design Files(*.vqm *.hex *.mif)"));
  }
  QStringList fileNames =
      QFileDialog::getOpenFileNames(this, tr("Select File"), "", fileformat);
  foreach (QString str, fileNames) {
    QString name = str.right(str.size() - (str.lastIndexOf("/") + 1));
    QString path = str.left(str.lastIndexOf("/"));
    QString suffix = name.right(name.size() - (name.lastIndexOf(".") + 1));

    filedata fdata;
    fdata.isfolder = 0;
    if ("v" == suffix) {
      fdata.filetype = ST_V;
    } else if ("vhd" == suffix) {
      fdata.filetype = ST_VHD;
    } else if ("CDC" == suffix) {
      fdata.filetype = ST_CDC;
    } else if ("vqm" == suffix) {
      fdata.filetype = ST_VQM;
    } else if ("hex" == suffix) {
      fdata.filetype = ST_HEX;
    } else if ("mif" == suffix) {
      fdata.filetype = ST_MIF;
    }
    fdata.strtype = suffix;
    fdata.fname = name;
    fdata.fpath = path;
    slot_addgridrow(fdata);
  }
}

void sourceGrid::slot_adddirectories() {
  QString pathName = QFileDialog::getExistingDirectory(
      this, tr("Select Directory"), "",
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if ("" == pathName) return;
  QString folder =
      pathName.right(pathName.size() - (pathName.lastIndexOf("/") + 1));
  QString path = pathName.left(pathName.lastIndexOf("/"));

  filedata fdata;
  fdata.isfolder = 1;
  fdata.fname = folder;
  fdata.fpath = path;
  slot_addgridrow(fdata);
}

void sourceGrid::slot_createfile() {
  //    createfiledialog* createdlg = new createfiledialog(this);
  //    createdlg->initialdialog(m_type);
  //    connect(createdlg,&createfiledialog::sig_updategrid,this,
  //    &sourceGrid::slot_addgridrow); createdlg->exec();
  //    disconnect(createdlg,&createfiledialog::sig_updategrid,this,
  //    &sourceGrid::slot_addgridrow);
}

void sourceGrid::slot_deletegriditem() {
  int curRow = m_selectmodel->currentIndex().row();
  if (curRow < 0) return;
  QString strtemp = m_model->item(curRow, 1)->text();
  for (int i = 0; i < m_fdatalist.count(); ++i) {
    if (m_fdatalist[i].fname == strtemp) {
      m_fdatalist.removeAt(i);
      break;
    }
  }
  m_model->removeRow(curRow);

  QStandardItemModel *model =
      qobject_cast<QStandardItemModel *>(m_grid->model());
  if (model == nullptr) {
    return;
  }
  int column = m_model->columnCount();
  int rows = m_model->rowCount();
  if (curRow == rows && rows != 0) curRow--;
  m_grid->setCurrentIndex(model->index(curRow, column));
  m_grid->selectRow(curRow);

  if (0 == rows) {
    m_actDel->setIcon(QIcon(":/img/img/del.png"));
    m_actDel->setEnabled(false);
  }
}

void sourceGrid::slot_upgriditem() {
  int curRow = m_selectmodel->currentIndex().row();
  if (curRow < 0) return;
  moverow(curRow, curRow - 1);
}

void sourceGrid::slot_downgriditem() {
  int curRow = m_selectmodel->currentIndex().row();
  if (curRow < 0) return;
  moverow(curRow, curRow + 1);
}

void sourceGrid::slot_selectionChanged() {
  int curRow = m_selectmodel->currentIndex().row();
  if (curRow < 0) return;

  int rows = m_model->rowCount();
  m_actDel->setIcon(QIcon(":/img/img/del_enable.png"));
  m_actDel->setEnabled(true);

  if (rows > curRow + 1) {
    m_actDown->setIcon(QIcon(":/img/img/down_enable.png"));
    m_actDown->setEnabled(true);
  } else {
    m_actDown->setIcon(QIcon(":/img/img/down.png"));
    m_actDown->setEnabled(false);
  }

  if (curRow > 0) {
    m_actUp->setIcon(QIcon(":/img/img/up_enable.png"));
    m_actUp->setEnabled(true);
  } else {
    m_actUp->setIcon(QIcon(":/img/img/up.png"));
    m_actUp->setEnabled(false);
  }
  return;
}

void sourceGrid::slot_addgridrow(filedata fdata) {
  if (isfiledataexit(fdata)) return;

  int rows = m_model->rowCount();
  QList<QStandardItem *> items;
  QStandardItem *item = nullptr;

  item = new QStandardItem(QString("%1").arg(rows + 1));
  item->setTextAlignment(Qt::AlignCenter);
  items.append(item);

  QIcon icon;
  if (0 == fdata.isfolder) {
    icon.addFile(":/img/img/file.png");
  } else {
    icon.addFile(":/img/img/files.png");
  }
  item = new QStandardItem();
  item->setIcon(icon);
  item->setText(fdata.fname);
  item->setTextAlignment(Qt::AlignLeft);
  items.append(item);

  item = new QStandardItem(fdata.fpath);
  item->setTextAlignment(Qt::AlignLeft);
  items.append(item);

  m_model->insertRow(rows, items);
  slot_selectionChanged();
  m_fdatalist.append(fdata);
}

void sourceGrid::moverow(int from, int to) {
  if (from == to) {
    return;
  }

  QStandardItemModel *model =
      qobject_cast<QStandardItemModel *>(m_grid->model());
  if (model == nullptr) {
    return;
  }

  QList<QStandardItem *> listItem = model->takeRow(from);
  model->insertRow(to, listItem);

  int column = m_grid->currentIndex().column();
  m_grid->setCurrentIndex(model->index(to, column));
  m_grid->selectRow(to);

  for (int i = 0; i < m_model->rowCount(); i++) {
    m_model->item(i, 0)->setText(QString::number(i + 1));
    m_model->item(i, 0)->setTextAlignment(Qt::AlignCenter);
  }
}

bool sourceGrid::isfiledataexit(filedata fdata) {
  foreach (filedata fd, m_fdatalist) {
    if (fdata.fname == fd.fname) {
      return true;
    }
  }
  return false;
}
