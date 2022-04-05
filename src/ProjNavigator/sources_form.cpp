#include "sources_form.h"

#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTextStream>

#include "tcl_command_integration.h"
#include "ui_sources_form.h"

using namespace FOEDAG;

SourcesForm::SourcesForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::SourcesForm) {
  ui->setupUi(this);

  m_treeSrcHierachy = new QTreeWidget(ui->m_tabHierarchy);
  m_treeSrcHierachy->setSelectionMode(
      QAbstractItemView::SelectionMode::SingleSelection);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_treeSrcHierachy);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);
  ui->m_tabHierarchy->setLayout(vbox);

  CreateActions();

  m_projManager = new ProjectManager(this);

  connect(m_treeSrcHierachy, SIGNAL(itemPressed(QTreeWidgetItem *, int)), this,
          SLOT(SlotItempressed(QTreeWidgetItem *, int)));
  connect(m_treeSrcHierachy, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
          this, SLOT(SlotItemDoubleClicked(QTreeWidgetItem *, int)));

  ui->m_tabWidget->removeTab(ui->m_tabWidget->indexOf(ui->tab_2));
}

SourcesForm::~SourcesForm() { delete ui; }

void SourcesForm::InitSourcesForm(const QString &strFile) {
  m_projManager->StartProject(strFile);

  UpdateSrcHierachyTree();
}

TclCommandIntegration *SourcesForm::createTclCommandIntegarion() {
  return new TclCommandIntegration(m_projManager, this);
}

void SourcesForm::TestOpenProject(int argc, const char *argv[]) {
  QTextStream out(stdout);
  if (argc < 3 || "--file" != QString(argv[1])) {
    out << "-----------open_project ------------\n";
    out << " \n";
    out << " Description: \n";
    out << " Open a project. Show the source file categories and hierarchies. "
           "\n";
    out << " \n";
    out << " Syntax: \n";
    out << " open_project --file <project.ospr> \n";
    out << " \n";
    out << "--------------------------------------\n";
    return;
  }

  QFileInfo fileInfo;
  fileInfo.setFile(QString(argv[2]));
  if (fileInfo.exists()) {
    m_projManager->StartProject(QString(argv[2]));
    UpdateSrcHierachyTree();
  } else {
    out << " Warning : This file <" << QString(argv[2]) << "> is not exist! \n";
  }
}

void SourcesForm::SetCurrentFileItem(const QString &strFileName) {
  QString filename = strFileName.right(strFileName.size() -
                                       (strFileName.lastIndexOf("/") + 1));
  QList<QTreeWidgetItem *> listItem = m_treeSrcHierachy->findItems(
      filename, Qt::MatchContains | Qt::MatchRecursive);
  foreach (auto item, listItem) {
    if (item == nullptr) {
      continue;
    }
    QString strItemFileName = (item->data(0, Qt::UserRole)).toString();
    QString strPath = m_projManager->getProjectPath();
    if (!strFileName.compare(strItemFileName.replace("$OSRCDIR", strPath))) {
      m_treeSrcHierachy->setCurrentItem(item);
      break;
    }
  }
}

void SourcesForm::SlotItempressed(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  if (qApp->mouseButtons() == Qt::RightButton) {
    QString strPropertyRole =
        (item->data(0, Qt::WhatsThisPropertyRole)).toString();
    QString strName = item->text(0);

    QMenu *menu = new QMenu(m_treeSrcHierachy);
    menu->setMinimumWidth(200);
    if (SRC_TREE_DESIGN_TOP_ITEM == strPropertyRole ||
        SRC_TREE_CONSTR_TOP_ITEM == strPropertyRole ||
        SRC_TREE_SIM_TOP_ITEM == strPropertyRole) {
      menu->addAction(m_actRefresh);
      menu->addSeparator();
      menu->addAction(m_actEditDesignSets);
      menu->addAction(m_actEditConstrsSets);
      menu->addAction(m_actEditSimulSets);
      menu->addSeparator();
      menu->addAction(m_actAddFile);

    } else if (SRC_TREE_DESIGN_SET_ITEM == strPropertyRole ||
               SRC_TREE_CONSTR_SET_ITEM == strPropertyRole ||
               SRC_TREE_SIM_SET_ITEM == strPropertyRole) {
      if (strName.contains(SRC_TREE_FLG_ACTIVE)) {
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditDesignSets);
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
      } else {
        menu->addAction(m_actRemoveFileset);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditDesignSets);
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
        menu->addAction(m_actMakeActive);
      }
    } else if (SRC_TREE_DESIGN_FILE_ITEM == strPropertyRole ||
               SRC_TREE_SIM_FILE_ITEM == strPropertyRole) {
      if (strName.contains(SRC_TREE_FLG_TOP)) {
        menu->addAction(m_actOpenFile);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditDesignSets);
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
      } else {
        menu->addAction(m_actOpenFile);
        menu->addAction(m_actRemoveFile);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditDesignSets);
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
        menu->addAction(m_actSetAsTop);
      }
    } else if (SRC_TREE_CONSTR_FILE_ITEM == strPropertyRole) {
      if (strName.contains(SRC_TREE_FLG_TARGET)) {
        menu->addAction(m_actOpenFile);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditDesignSets);
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
      } else {
        menu->addAction(m_actOpenFile);
        menu->addAction(m_actRemoveFile);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditDesignSets);
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
        menu->addAction(m_actSetAsTarget);
      }
    }

    QPoint p = QCursor::pos();
    menu->exec(QPoint(p.rx(), p.ry() + 3));
    menu->deleteLater();
  }
}

void SourcesForm::SlotItemDoubleClicked(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column);
  QString strPropertyRole =
      (item->data(0, Qt::WhatsThisPropertyRole)).toString();
  if (SRC_TREE_DESIGN_FILE_ITEM == strPropertyRole ||
      SRC_TREE_SIM_FILE_ITEM == strPropertyRole ||
      SRC_TREE_CONSTR_FILE_ITEM == strPropertyRole) {
    SlotOpenFile();
  }
}

void SourcesForm::SlotRefreshSourceTree() { UpdateSrcHierachyTree(); }

void SourcesForm::SlotCreateDesignSet() {
  CreateFileSetDialog *createdialog = new CreateFileSetDialog(this);
  createdialog->InitDialog(FST_DESIGN);

  while (createdialog->exec()) {
    QString strName = createdialog->getDesignName();
    int ret = m_projManager->setDesignFileSet(strName);
    if (1 == ret) {
      QMessageBox::information(this, tr("Information"),
                               tr("The set name is already exists!"),
                               QMessageBox::Ok);
    } else if (0 == ret) {
      UpdateSrcHierachyTree();
      m_projManager->FinishedProject();
      break;
    }
  }
  createdialog->close();
  createdialog->deleteLater();
}

void SourcesForm::SlotCreateConstrSet() {
  CreateFileSetDialog *createdialog = new CreateFileSetDialog(this);
  createdialog->InitDialog(FST_CONSTR);

  while (createdialog->exec()) {
    QString strName = createdialog->getDesignName();
    int ret = m_projManager->setConstrFileSet(strName);
    if (1 == ret) {
      QMessageBox::information(this, tr("Information"),
                               tr("The set name is already exists!"),
                               QMessageBox::Ok);
    } else if (0 == ret) {
      UpdateSrcHierachyTree();
      m_projManager->FinishedProject();
      break;
    }
  }
  createdialog->close();
  createdialog->deleteLater();
}

void SourcesForm::SlotCreateSimSet() {
  CreateFileSetDialog *createdialog = new CreateFileSetDialog(this);
  createdialog->InitDialog(FST_SIM);

  while (createdialog->exec()) {
    QString strName = createdialog->getDesignName();
    int ret = m_projManager->setSimulationFileSet(strName);
    if (1 == ret) {
      QMessageBox::information(this, tr("Information"),
                               tr("The set name is already exists!"),
                               QMessageBox::Ok);
    } else if (0 == ret) {
      UpdateSrcHierachyTree();
      m_projManager->FinishedProject();
      break;
    }
  }
  createdialog->close();
  createdialog->deleteLater();
}

void SourcesForm::SlotAddFile() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }

  QString strPropertyRole =
      (item->data(0, Qt::WhatsThisPropertyRole)).toString();
  QString strFielSetName = (item->data(0, Qt::UserRole)).toString();

  AddFileDialog *addFileDialog = new AddFileDialog(this);
  if (SRC_TREE_DESIGN_SET_ITEM == strPropertyRole ||
      SRC_TREE_DESIGN_TOP_ITEM == strPropertyRole ||
      SRC_TREE_DESIGN_FILE_ITEM == strPropertyRole) {
    addFileDialog->setSelected(GT_SOURCE);
  } else if (SRC_TREE_CONSTR_SET_ITEM == strPropertyRole ||
             SRC_TREE_CONSTR_TOP_ITEM == strPropertyRole ||
             SRC_TREE_CONSTR_FILE_ITEM == strPropertyRole) {
    addFileDialog->setSelected(GT_CONSTRAINTS);
  } else if (SRC_TREE_SIM_SET_ITEM == strPropertyRole ||
             SRC_TREE_SIM_TOP_ITEM == strPropertyRole ||
             SRC_TREE_SIM_FILE_ITEM == strPropertyRole) {
    addFileDialog->setSelected(GT_SIM);
  } else {
    addFileDialog->deleteLater();
    return;
  }
  connect(addFileDialog, SIGNAL(RefreshFiles()), this,
          SLOT(SlotRefreshSourceTree()));
  addFileDialog->exec();

  addFileDialog->close();

  disconnect(addFileDialog, SIGNAL(RefreshFiles()), this,
             SLOT(SlotRefreshSourceTree()));
}

void SourcesForm::SlotOpenFile() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strFileName = (item->data(0, Qt::UserRole)).toString();

  QString strPath = m_projManager->getProjectPath();

  emit OpenFile(strFileName.replace(PROJECT_OSRCDIR, strPath));
}

void SourcesForm::SlotRemoveFileSet() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strName = (item->data(0, Qt::UserRole)).toString();

  int ret = m_projManager->deleteFileSet(strName);
  if (0 == ret) {
    UpdateSrcHierachyTree();
    m_projManager->FinishedProject();
  }
}

void SourcesForm::SlotRemoveFile() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strFileName = item->text(0);

  QTreeWidgetItem *itemparent = item->parent();
  QString strFileSetName = (itemparent->data(0, Qt::UserRole)).toString();

  m_projManager->setCurrentFileSet(strFileSetName);
  int ret = m_projManager->deleteFile(strFileName);
  if (0 == ret) {
    UpdateSrcHierachyTree();
    m_projManager->FinishedProject();
  }
}

void SourcesForm::SlotSetAsTop() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strFileName = item->text(0);

  QTreeWidgetItem *itemparent = item->parent();
  QString strFileSetName = (itemparent->data(0, Qt::UserRole)).toString();

  m_projManager->setCurrentFileSet(strFileSetName);
  QString module = strFileName.left(strFileName.lastIndexOf("."));
  int ret = m_projManager->setTopModule(module);
  if (0 == ret) {
    UpdateSrcHierachyTree();
    m_projManager->FinishedProject();
  }
}

void SourcesForm::SlotSetAsTarget() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }
  QString strFileName = item->text(0);

  QTreeWidgetItem *itemparent = item->parent();
  QString strFileSetName = (itemparent->data(0, Qt::UserRole)).toString();

  m_projManager->setCurrentFileSet(strFileSetName);
  int ret = m_projManager->setTargetConstrs(strFileName);
  if (0 == ret) {
    UpdateSrcHierachyTree();
    m_projManager->FinishedProject();
  }
}

void SourcesForm::SlotSetActive() {
  int ret = 0;
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }

  QString strPropertyRole =
      (item->data(0, Qt::WhatsThisPropertyRole)).toString();
  QString strName = (item->data(0, Qt::UserRole)).toString();

  if (SRC_TREE_DESIGN_SET_ITEM == strPropertyRole) {
    ret = m_projManager->setDesignActive(strName);
  } else if (SRC_TREE_CONSTR_SET_ITEM == strPropertyRole) {
    ret = m_projManager->setConstrActive(strName);
  } else if (SRC_TREE_SIM_SET_ITEM == strPropertyRole) {
    ret = m_projManager->setSimulationActive(strName);
  } else {
    return;
  }

  if (0 == ret) {
    UpdateSrcHierachyTree();
    m_projManager->FinishedProject();
  }
}

void SourcesForm::CreateActions() {
  m_actRefresh = new QAction(tr("Refresh Hierarchy"), m_treeSrcHierachy);
  connect(m_actRefresh, SIGNAL(triggered()), this,
          SLOT(SlotRefreshSourceTree()));

  m_actEditDesignSets = new QAction(tr("Create Design Set"), m_treeSrcHierachy);
  connect(m_actEditDesignSets, SIGNAL(triggered()), this,
          SLOT(SlotCreateDesignSet()));

  m_actEditConstrsSets =
      new QAction(tr("Create Constraints Set"), m_treeSrcHierachy);
  connect(m_actEditConstrsSets, SIGNAL(triggered()), this,
          SLOT(SlotCreateConstrSet()));

  m_actEditSimulSets =
      new QAction(tr("Create Simulation Set"), m_treeSrcHierachy);
  connect(m_actEditSimulSets, SIGNAL(triggered()), this,
          SLOT(SlotCreateSimSet()));

  m_actAddFile = new QAction(tr("Add Sources"), m_treeSrcHierachy);
  m_actAddFile->setIcon(QIcon(":/images/add.png"));
  connect(m_actAddFile, SIGNAL(triggered()), this, SLOT(SlotAddFile()));

  m_actOpenFile = new QAction(tr("Open File"), m_treeSrcHierachy);
  connect(m_actOpenFile, SIGNAL(triggered()), this, SLOT(SlotOpenFile()));

  m_actRemoveFileset = new QAction(tr("Remove File Set"), m_treeSrcHierachy);
  connect(m_actRemoveFileset, SIGNAL(triggered()), this,
          SLOT(SlotRemoveFileSet()));

  m_actRemoveFile = new QAction(tr("Remove File"), m_treeSrcHierachy);
  connect(m_actRemoveFile, SIGNAL(triggered()), this, SLOT(SlotRemoveFile()));

  m_actSetAsTop = new QAction(tr("Set As TopModule"), m_treeSrcHierachy);
  connect(m_actSetAsTop, SIGNAL(triggered()), this, SLOT(SlotSetAsTop()));

  m_actSetAsTarget =
      new QAction(tr("Set as Target Constraint File"), m_treeSrcHierachy);
  connect(m_actSetAsTarget, SIGNAL(triggered()), this, SLOT(SlotSetAsTarget()));

  m_actMakeActive = new QAction(tr("Make Active"), m_treeSrcHierachy);
  connect(m_actMakeActive, SIGNAL(triggered()), this, SLOT(SlotSetActive()));
}

void SourcesForm::UpdateSrcHierachyTree() {
  if (nullptr == m_projManager) {
    return;
  }

  m_treeSrcHierachy->clear();

  // Initialize design sources tree
  QTreeWidgetItem *topItem = new QTreeWidgetItem(m_treeSrcHierachy);
  const QString topItemName = m_projManager->getProjectName().isEmpty()
                                  ? "undefined"
                                  : m_projManager->getProjectName();
  topItem->setText(0, topItemName);
  m_treeSrcHierachy->addTopLevelItem(topItem);

  QTreeWidgetItem *topitemDS = new QTreeWidgetItem(topItem);
  topitemDS->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_DESIGN_TOP_ITEM);

  QStringList listDesFset = m_projManager->getDesignFileSets();
  QString strDesAct = m_projManager->getDesignActiveFileSet();
  int iFileSum = 0;
  foreach (auto str, listDesFset) {
    QTreeWidgetItem *itemfolder = new QTreeWidgetItem(topitemDS);
    itemfolder->setIcon(0, QIcon(":/images/open-file.png"));
    itemfolder->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_DESIGN_SET_ITEM);

    QStringList listDesFile = m_projManager->getDesignFiles(str);
    QString strTop = m_projManager->getDesignTopModule(str);

    foreach (auto strfile, listDesFile) {
      QString filename =
          strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
      QTreeWidgetItem *itemf = new QTreeWidgetItem(itemfolder);
      QString module = filename.left(filename.lastIndexOf("."));
      if (module == strTop) {
        itemf->setText(0, filename + SRC_TREE_FLG_TOP);
      } else {
        itemf->setText(0, filename);
      }
      itemf->setIcon(0, QIcon(":/img/file.png"));
      itemf->setData(0, Qt::UserRole, strfile);
      itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_DESIGN_FILE_ITEM);
    }

    QString strNum = QString("(%1)").arg(listDesFile.size());
    iFileSum += listDesFile.size();
    if (str == strDesAct) {
      itemfolder->setText(0, str + strNum + SRC_TREE_FLG_ACTIVE);
    } else {
      itemfolder->setText(0, str + strNum);
    }
    itemfolder->setData(0, Qt::UserRole, str);
  }
  topitemDS->setText(0, tr("Design Sources") + QString("(%1)").arg(iFileSum));

  // Initialize Constraints sources tree
  QTreeWidgetItem *topitemCS = new QTreeWidgetItem(topItem);
  m_treeSrcHierachy->addTopLevelItem(topitemCS);
  topitemCS->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_CONSTR_TOP_ITEM);

  QStringList listConstrFset = m_projManager->getConstrFileSets();
  QString strConstrAct = m_projManager->getConstrActiveFileSet();
  iFileSum = 0;
  foreach (auto str, listConstrFset) {
    QTreeWidgetItem *itemfolder = new QTreeWidgetItem(topitemCS);
    itemfolder->setIcon(0, QIcon(":/images/open-file.png"));
    itemfolder->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_CONSTR_SET_ITEM);

    QStringList listConstrFile = m_projManager->getConstrFiles(str);
    QString strTarget = m_projManager->getConstrTargetFile(str);

    foreach (auto strfile, listConstrFile) {
      QString filename =
          strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
      QTreeWidgetItem *itemf = new QTreeWidgetItem(itemfolder);
      if (filename == strTarget) {
        itemf->setText(0, filename + SRC_TREE_FLG_TARGET);
      } else {
        itemf->setText(0, filename);
      }
      itemf->setData(0, Qt::UserRole, strfile);
      itemf->setIcon(0, QIcon(":/img/file.png"));
      itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_CONSTR_FILE_ITEM);
    }

    QString strNum = QString("(%1)").arg(listConstrFile.size());
    iFileSum += listConstrFile.size();
    if (str == strConstrAct) {
      itemfolder->setText(0, str + strNum + SRC_TREE_FLG_ACTIVE);
    } else {
      itemfolder->setText(0, str + strNum);
    }
    itemfolder->setData(0, Qt::UserRole, str);
  }
  topitemCS->setText(0, tr("Constraints") + QString("(%1)").arg(iFileSum));

  // Initialize simulation sources tree
  QTreeWidgetItem *topitemSS = new QTreeWidgetItem(topItem);
  m_treeSrcHierachy->addTopLevelItem(topitemSS);
  topitemSS->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_SIM_TOP_ITEM);

  QStringList listSimFset = m_projManager->getSimulationFileSets();
  QString strSimAct = m_projManager->getSimulationActiveFileSet();
  iFileSum = 0;
  foreach (auto str, listSimFset) {
    QTreeWidgetItem *itemfolder = new QTreeWidgetItem(topitemSS);
    itemfolder->setIcon(0, QIcon(":/images/open-file.png"));
    itemfolder->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_SIM_SET_ITEM);

    QStringList listSimFile = m_projManager->getSimulationFiles(str);
    QString strTop = m_projManager->getSimulationTopModule(str);

    foreach (auto strfile, listSimFile) {
      QString filename =
          strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
      QTreeWidgetItem *itemf = new QTreeWidgetItem(itemfolder);
      QString module = filename.left(filename.lastIndexOf("."));
      if (module == strTop) {
        itemf->setText(0, filename + SRC_TREE_FLG_TOP);
      } else {
        itemf->setText(0, filename);
      }
      itemf->setData(0, Qt::UserRole, strfile);
      itemf->setIcon(0, QIcon(":/img/file.png"));
      itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_SIM_FILE_ITEM);
    }

    QString strNum = QString("(%1)").arg(listSimFile.size());
    iFileSum += listSimFile.size();
    if (str == strSimAct) {
      itemfolder->setText(0, str + strNum + SRC_TREE_FLG_ACTIVE);
    } else {
      itemfolder->setText(0, str + strNum);
    }
    itemfolder->setData(0, Qt::UserRole, str);
  }
  topitemSS->setText(0,
                     tr("Simulation Sources") + QString("(%1)").arg(iFileSum));

  m_treeSrcHierachy->setHeaderHidden(true);
  m_treeSrcHierachy->expandAll();
}
