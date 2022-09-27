#include "source_grid.h"

#include <QDirIterator>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QStandardItem>
#include <QVBoxLayout>

#include "ProjectManager/project_manager.h"
#include "create_file_dialog.h"
using namespace FOEDAG;

namespace {
static const auto INDEX_COL = QObject::tr("Index");
static const auto NAME_COL = QObject::tr("Name");
static const auto LIBRARY_COL = QObject::tr("Library");
static const auto LOCATION_COL = QObject::tr("Location");

static const auto CONSTR_FILTER = QObject::tr("Constraint Files(*.sdc)");
static const auto DESIGN_SOURCES_FILTER = QObject::tr(
    "Design Source Files (*.vhd *.vhdl *.vhf *.vhdp *.vho *.v *.vf *.verilog "
    "*.vr *.vg *.vb *.tf *.vlog *.vp *.vm *.veo *.svo *.vh *.h *.svh *.vhp "
    "*.svhp *.edf *.edif *.sv *.svp *.bmm *.mif *.mem *.elf);;"
    "AES Key Files (*.nky *.nkz);;"
    "VHDL Files (*.vho *.vhd *.vhdl *.vhf *.vhdp);;"
    "VERILOG Files (*.sv *.svp *.v *.veo *.svo *.vf *.verilog *.vr *.vg *.vb "
    "*.tf *.vlog *.vp *.vm);;"
    "VERILOG Header Files(*.vh *.h *.svh *.vhp *.svhp);;"
    "NETLIST files (*.edif *.edf *.eblif *.blif *.v *.sv *.svp);;"
    "HDL Files (*.vhd *.vhdl *.vhf *.vhdp *.vho *.v *.vf *.verilog *.vr *.vg "
    "*.vb *.tf *.vlog *.vp *.vm *.veo *.svo *.vh *.h *.svh *.vhp *.svhp *.sv "
    "*.svp)");
}  // namespace

sourceGrid::sourceGrid(QWidget *parent) : QWidget(parent) {
  m_lisFileData.clear();
  m_btnAddFile = new QPushButton(tr("Add File..."), this);
  connect(m_btnAddFile, &QPushButton::clicked, this, &sourceGrid::AddFiles);
  m_btnAddDri = new QPushButton(tr("Add Directory"
                                   "..."),
                                this);
  connect(m_btnAddDri, &QPushButton::clicked, this,
          &sourceGrid::AddDirectories);
  m_btnCreateFile = new QPushButton(tr("Create File..."), this);
  connect(m_btnCreateFile, &QPushButton::clicked, this,
          &sourceGrid::CreateFile);
  m_btnDelete = new QPushButton(tr("Remove"), this);
  m_btnDelete->setEnabled(false);
  connect(m_btnDelete, &QPushButton::clicked, this,
          &sourceGrid::DeleteTableItem);
  m_btnMoveUp = new QPushButton(tr("Move Up"), this);
  m_btnMoveUp->setEnabled(false);
  connect(m_btnMoveUp, &QPushButton::clicked, this, &sourceGrid::UpTableItem);
  m_btnMoveDown = new QPushButton(tr("Move Down"), this);
  m_btnMoveDown->setEnabled(false);
  connect(m_btnMoveDown, &QPushButton::clicked, this,
          &sourceGrid::DownTableItem);

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->addWidget(m_btnAddFile);
  hbox->addWidget(m_btnAddDri);
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
  // Last column adaptive width
  m_tableViewSrc->horizontalHeader()->setStretchLastSection(true);
  m_tableViewSrc->setSelectionBehavior(QTableView::SelectRows);
  // Single line selection
  m_tableViewSrc->setSelectionMode(QTableView::SingleSelection);
  // Color separation between lines
  m_tableViewSrc->setAlternatingRowColors(true);

  m_tableViewSrc->setStyleSheet(
      "QTableView {border: 1px solid rgb(230,230,230);}\
       QTableView::item:selected{color:black;background:rgb(177,220,255);}");
  m_tableViewSrc->setColumnWidth(0, 80);

  m_model = new QStandardItemModel(m_tableViewSrc);
  m_selectModel = new QItemSelectionModel(m_model, m_model);

  connect(m_model, &QStandardItemModel::itemChanged, this,
          &sourceGrid::onItemChanged);

  m_tableViewSrc->horizontalHeader()->setMinimumHeight(30);

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
void sourceGrid::setGridType(GridType type) {
  m_type = type;

  auto columnIndex{0};
  m_model->setHorizontalHeaderItem(columnIndex++, new QStandardItem(INDEX_COL));
  m_model->setHorizontalHeaderItem(columnIndex++, new QStandardItem(NAME_COL));
  if (GT_SOURCE == m_type) {
    m_model->setHorizontalHeaderItem(columnIndex++,
                                     new QStandardItem(LIBRARY_COL));
  }

  m_model->setHorizontalHeaderItem(columnIndex++,
                                   new QStandardItem(LOCATION_COL));

  if (GT_CONSTRAINTS == m_type) {
    m_btnAddDri->setVisible(false);
  } else {
    m_btnAddDri->setVisible(true);
  }
}

QList<filedata> sourceGrid::getTableViewData() { return m_lisFileData; }

void sourceGrid::currentFileSet(const QString &fileSet) {
  m_currentFileSet = fileSet;
}

void sourceGrid::AddFiles() {
  QString fileformat{DESIGN_SOURCES_FILTER};
  if (GT_CONSTRAINTS == m_type) fileformat = CONSTR_FILTER;
  // this option will catch lower and upper cases extentions
  auto option{QFileDialog::DontUseNativeDialog};
  QStringList fileNames = QFileDialog::getOpenFileNames(
      this, tr("Select File"), "", fileformat, nullptr, option);
  for (const QString &str : fileNames) {
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
  auto folder = QFileDialog::getExistingDirectory(
      this, tr("Select Directory"), "",
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks |
          QFileDialog::DontUseNativeDialog);

  if (folder.isEmpty())  // The dialog was cancelled
    return;
  auto it = QDirIterator(folder, GetAllDesignSourceExtentions(), QDir::NoFilter,
                         QDirIterator::Subdirectories);
  auto files =
      std::vector<std::pair<QString, QString>>{};  // File names with directory
                                                   // full paths
  while (it.hasNext()) {
    it.next();
    files.emplace_back(it.fileName(), it.path());
  }

  for (auto &[fileName, filePath] : files) {
    auto suffix =
        fileName.right(fileName.size() - (fileName.lastIndexOf(".") + 1));
    filedata fdata;
    fdata.m_isFolder = false;
    fdata.m_fileType = suffix;
    fdata.m_fileName = fileName;
    fdata.m_filePath = filePath;
    AddTableItem(fdata);
  }
}

void sourceGrid::CreateFile() {
  auto path = ProjectManager::ProjectFilesPath(
      Project::Instance()->projectPath(), Project::Instance()->projectName(),
      m_currentFileSet);
  if (Project::Instance()->projectPath().isEmpty()) path = QString();
  createFileDialog *createdlg = new createFileDialog(path, this);
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
  item->setEditable(false);
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
  item->setEditable(false);
  items.append(item);

  if (GT_SOURCE == m_type) {
    item = new QStandardItem(fdata.m_workLibrary);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    item->setEditable(true);
    items.append(item);
  }

  item = new QStandardItem(fdata.m_filePath);
  item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  item->setEditable(false);
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

  if ((from >= 0 && from < m_lisFileData.size()) &&
      (to >= 0 && to < m_lisFileData.size()))
    m_lisFileData.move(from, to);
  else
    qWarning("m_lisFileData: wrong indexes!");
}

bool sourceGrid::IsFileDataExit(filedata fdata) {
  foreach (filedata fd, m_lisFileData) {
    if (fdata.m_fileName == fd.m_fileName) {
      return true;
    }
  }
  return false;
}

void sourceGrid::onItemChanged(QStandardItem *item) {
  auto itemIndex = m_model->indexFromItem(item);

  if (itemIndex.row() >= m_lisFileData.size())
    qWarning("m_lisFileData: wrong indexes!");

  if (m_model->headerData(itemIndex.column(), Qt::Horizontal).toString() ==
      LIBRARY_COL)
    m_lisFileData[itemIndex.row()].m_workLibrary = item->text();
}

QStringList sourceGrid::GetAllDesignSourceExtentions() const {
  QSet<QString> filters;
  auto filterLine = DESIGN_SOURCES_FILTER.split(";;");
  for (auto &f : filterLine) {
    f.remove(0, f.indexOf("(") + 1);
    f = f.mid(0, f.indexOf(")"));
    const auto ext = f.split(" ");
    for (const auto &e : ext) {
      filters.insert(e);
      filters.insert(e.toUpper());
    }
  }
  return filters.values();
}
