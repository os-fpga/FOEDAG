#include "sources_form.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTextStream>
#include <algorithm>

#include "Main/Foedag.h"
#include "tcl_command_integration.h"
#include "ui_sources_form.h"

using namespace FOEDAG;
static constexpr int SetFileDataRole{Qt::UserRole + 1};

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

void SourcesForm::InitSourcesForm() { UpdateSrcHierachyTree(); }

TclCommandIntegration *SourcesForm::createTclCommandIntegarion() {
  return new TclCommandIntegration(m_projManager, this);
}

ProjectManager *SourcesForm::ProjManager() { return m_projManager; }

void SourcesForm::CreateConstraint() { showAddFileDialog(GT_CONSTRAINTS); }

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
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
      } else {
        menu->addAction(m_actRemoveFileset);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
        menu->addAction(m_actMakeActive);
      }
    } else if (SRC_TREE_DESIGN_FILE_ITEM == strPropertyRole ||
               SRC_TREE_SIM_FILE_ITEM == strPropertyRole) {
      menu->addAction(m_actOpenFile);
      menu->addAction(m_actRemoveFile);
      menu->addAction(m_actRefresh);
      menu->addSeparator();
      menu->addAction(m_actEditConstrsSets);
      menu->addAction(m_actEditSimulSets);
      menu->addSeparator();
      menu->addAction(m_actAddFile);
    } else if (SRC_TREE_CONSTR_FILE_ITEM == strPropertyRole) {
      if (strName.contains(SRC_TREE_FLG_TARGET)) {
        menu->addAction(m_actOpenFile);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
      } else {
        menu->addAction(m_actOpenFile);
        menu->addAction(m_actRemoveFile);
        menu->addAction(m_actRefresh);
        menu->addSeparator();
        menu->addAction(m_actEditConstrsSets);
        menu->addAction(m_actEditSimulSets);
        menu->addSeparator();
        menu->addAction(m_actAddFile);
        menu->addAction(m_actSetAsTarget);
      }
    } else if (SRC_TREE_IP_FILE_ITEM == strPropertyRole ||
               SRC_TREE_IP_TOP_ITEM == strPropertyRole) {
      menu->addAction(m_actRefresh);
      if (SRC_TREE_IP_FILE_ITEM == strPropertyRole) {
        menu->addSeparator();
        menu->addAction(m_actReconfigureIp);
      }
    }

    if (m_projManager->HasDesign()) {
      menu->addSeparator();
      menu->addAction(m_actCloseProject);
    }

    if (SRC_TREE_CONSTR_FILE_ITEM == strPropertyRole ||
        SRC_TREE_DESIGN_FILE_ITEM == strPropertyRole ||
        SRC_TREE_SIM_FILE_ITEM == strPropertyRole) {
      menu->addSeparator();
      menu->addAction(m_actProperties);
    }

    QPoint p = QCursor::pos();
    menu->exec(QPoint(p.rx(), p.ry() + 3));
    menu->deleteLater();
  } else if (qApp->mouseButtons() == Qt::LeftButton) {
    SlotProperties();
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

  if (SRC_TREE_DESIGN_SET_ITEM == strPropertyRole ||
      SRC_TREE_DESIGN_TOP_ITEM == strPropertyRole ||
      SRC_TREE_DESIGN_FILE_ITEM == strPropertyRole) {
    showAddFileDialog(GT_SOURCE);
  } else if (SRC_TREE_CONSTR_SET_ITEM == strPropertyRole ||
             SRC_TREE_CONSTR_TOP_ITEM == strPropertyRole ||
             SRC_TREE_CONSTR_FILE_ITEM == strPropertyRole) {
    showAddFileDialog(GT_CONSTRAINTS);
  } else if (SRC_TREE_SIM_SET_ITEM == strPropertyRole ||
             SRC_TREE_SIM_TOP_ITEM == strPropertyRole ||
             SRC_TREE_SIM_FILE_ITEM == strPropertyRole) {
    showAddFileDialog(GT_SIM);
  }
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
  if (item == nullptr) return;

  auto strFileName = item->text(0);
  auto fileSet = item->data(0, SetFileDataRole);
  if (!fileSet.isNull()) {
    auto questionStr =
        tr("Are you sure you want to remove %1 from the project? \n\nThe file "
           "will not be removed from the disk.")
            .arg(strFileName);
    if (QMessageBox::question(this, {}, questionStr) == QMessageBox::No) return;
    m_projManager->setCurrentFileSet(fileSet.toString());
    int ret = m_projManager->deleteFile(strFileName);
    if (0 == ret) {
      UpdateSrcHierachyTree();
      m_projManager->FinishedProject();
    }
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

void SourcesForm::SlotProperties() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) return;

  QString strFileName = (item->data(0, Qt::UserRole)).toString();

  if (!strFileName.isEmpty()) emit ShowProperty(strFileName);
}

void SourcesForm::SlotPropertiesTriggered() {
  emit ShowPropertyPanel();
  SlotProperties();
}

void SourcesForm::SlotReConfigureIp() {
  QTreeWidgetItem *item = m_treeSrcHierachy->currentItem();
  if (item == nullptr) {
    return;
  }
  std::string moduleName = item->text(0).toStdString();

  auto instances =
      GlobalSession->GetCompiler()->GetIPGenerator()->IPInstances();

  // Find the ip instance with a matching module name
  auto isMatch = [moduleName](IPInstance *instance) {
    return instance->ModuleName() == moduleName;
  };
  auto result =
      std::find_if(std::begin(instances), std::end(instances), isMatch);

  std::string ipName{};
  QStringList args{};
  if (result != std::end(instances)) {
    ipName = (*result)->IPName();

    // Step through this instance's paremeters
    for (auto param : (*result)->Parameters()) {
      // Create a list of parameter value pairs in the format of
      // -P<param_name> <value>
      args.append(QString("-P%1 %2")
                      .arg(QString::fromStdString(param.Name()))
                      .arg(QString::number(param.GetValue())));
    }
  }

  emit IpReconfigRequested(QString::fromStdString(ipName),
                           QString::fromStdString(moduleName), args);
}

void SourcesForm::CreateActions() {
  m_actRefresh = new QAction(tr("Refresh Hierarchy"), m_treeSrcHierachy);
  connect(m_actRefresh, SIGNAL(triggered()), this,
          SLOT(SlotRefreshSourceTree()));

  m_actEditConstrsSets =
      new QAction(tr("Create Constraints Set..."), m_treeSrcHierachy);
  connect(m_actEditConstrsSets, SIGNAL(triggered()), this,
          SLOT(SlotCreateConstrSet()));

  m_actEditSimulSets =
      new QAction(tr("Create Simulation Set..."), m_treeSrcHierachy);
  connect(m_actEditSimulSets, SIGNAL(triggered()), this,
          SLOT(SlotCreateSimSet()));

  m_actAddFile = new QAction(tr("Add Sources..."), m_treeSrcHierachy);
  m_actAddFile->setIcon(QIcon(":/images/add.png"));
  connect(m_actAddFile, SIGNAL(triggered()), this, SLOT(SlotAddFile()));

  m_actOpenFile = new QAction(tr("Open File"), m_treeSrcHierachy);
  connect(m_actOpenFile, SIGNAL(triggered()), this, SLOT(SlotOpenFile()));

  m_actRemoveFileset = new QAction(tr("Remove File Set"), m_treeSrcHierachy);
  connect(m_actRemoveFileset, SIGNAL(triggered()), this,
          SLOT(SlotRemoveFileSet()));

  m_actRemoveFile = new QAction(tr("Remove File"), m_treeSrcHierachy);
  m_actRemoveFile->setShortcut(Qt::Key_Delete);
  m_treeSrcHierachy->addAction(m_actRemoveFile);
  connect(m_actRemoveFile, SIGNAL(triggered()), this, SLOT(SlotRemoveFile()));

  m_actSetAsTarget =
      new QAction(tr("Set as Target Constraint File"), m_treeSrcHierachy);
  connect(m_actSetAsTarget, SIGNAL(triggered()), this, SLOT(SlotSetAsTarget()));

  m_actMakeActive = new QAction(tr("Make Active"), m_treeSrcHierachy);
  connect(m_actMakeActive, SIGNAL(triggered()), this, SLOT(SlotSetActive()));

  m_actProperties = new QAction(tr("Properties..."), m_treeSrcHierachy);
  connect(m_actProperties, SIGNAL(triggered()), this,
          SLOT(SlotPropertiesTriggered()));

  m_actCloseProject = new QAction(tr("Close project"), m_treeSrcHierachy);
  connect(m_actCloseProject, SIGNAL(triggered()), this, SIGNAL(CloseProject()));

  m_actReconfigureIp = new QAction(tr("Reconfigure IP"), m_treeSrcHierachy);
  connect(m_actReconfigureIp, &QAction::triggered, this,
          &SourcesForm::SlotReConfigureIp);
}

void SourcesForm::UpdateSrcHierachyTree() {
  if (nullptr == m_projManager) return;
  m_treeSrcHierachy->clear();
  CreateFolderHierachyTree();
  m_treeSrcHierachy->setHeaderHidden(true);
  m_treeSrcHierachy->expandAll();
}

void SourcesForm::CreateFolderHierachyTree() {
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
  int iFileSum = 0;
  for (auto &str : listDesFset) {
    QStringList listDesFile = m_projManager->getDesignFiles(str);
    QTreeWidgetItem *parentItem{topitemDS};
    for (auto &strfile : listDesFile) {
      if (parentItem) {
        QString filename =
            strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
        QTreeWidgetItem *itemf = new QTreeWidgetItem(parentItem);
        itemf->setText(0, filename);
        itemf->setData(0, Qt::UserRole, strfile);
        itemf->setIcon(0, QIcon(":/img/file.png"));
        itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_DESIGN_FILE_ITEM);
        itemf->setData(0, SetFileDataRole, str);
      }
    }
    iFileSum += listDesFile.size();
  }
  topitemDS->setText(0, tr("Design Sources") + QString("(%1)").arg(iFileSum));

  // Initialize Constraints sources tree
  QTreeWidgetItem *topitemCS = new QTreeWidgetItem(topItem);
  m_treeSrcHierachy->addTopLevelItem(topitemCS);
  topitemCS->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_CONSTR_TOP_ITEM);

  QStringList listConstrFset = m_projManager->getConstrFileSets();
  iFileSum = 0;
  QTreeWidgetItem *parentItem{topitemCS};
  for (auto &str : listConstrFset) {
    QStringList listConstrFile = m_projManager->getConstrFiles(str);
    QString strTarget = m_projManager->getConstrTargetFile(str);
    for (auto &strfile : listConstrFile) {
      if (parentItem) {
        QString filename =
            strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
        QTreeWidgetItem *itemf = new QTreeWidgetItem(parentItem);
        if (filename == strTarget) {
          itemf->setText(0, filename + SRC_TREE_FLG_TARGET);
        } else {
          itemf->setText(0, filename);
        }
        itemf->setData(0, Qt::UserRole, strfile);
        itemf->setIcon(0, QIcon(":/img/file.png"));
        itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_CONSTR_FILE_ITEM);
        itemf->setData(0, SetFileDataRole, str);
      }
    }
    iFileSum += listConstrFile.size();
  }
  topitemCS->setText(0, tr("Constraints") + QString("(%1)").arg(iFileSum));

  // Initialize simulation sources tree
  QTreeWidgetItem *topitemSS = new QTreeWidgetItem(topItem);
  m_treeSrcHierachy->addTopLevelItem(topitemSS);
  topitemSS->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_SIM_TOP_ITEM);

  QStringList listSimFset = m_projManager->getSimulationFileSets();
  iFileSum = 0;
  for (const auto &str : listSimFset) {
    QStringList listSimFile = m_projManager->getSimulationFiles(str);
    QTreeWidgetItem *parentItem{topitemSS};
    for (auto &strfile : listSimFile) {
      if (parentItem) {
        QString filename =
            strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
        QTreeWidgetItem *itemf = new QTreeWidgetItem(parentItem);
        itemf->setText(0, filename);
        itemf->setData(0, Qt::UserRole, strfile);
        itemf->setIcon(0, QIcon(":/img/file.png"));
        itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_SIM_FILE_ITEM);
        itemf->setData(0, SetFileDataRole, str);
      }
    }
    iFileSum += listSimFile.size();
  }
  topitemSS->setText(0,
                     tr("Simulation Sources") + QString("(%1)").arg(iFileSum));

  // Initialize IP instances tree
  QTreeWidgetItem *topitemIpInstances = new QTreeWidgetItem(topItem);
  m_treeSrcHierachy->addTopLevelItem(topitemIpInstances);
  topitemIpInstances->setData(0, Qt::WhatsThisPropertyRole,
                              SRC_TREE_IP_TOP_ITEM);

  Compiler *compiler = nullptr;
  IPGenerator *ipGen = nullptr;
  if (GlobalSession && (compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    iFileSum = 0;
    QTreeWidgetItem *ipParentItem{topitemIpInstances};
    for (auto instance : ipGen->IPInstances()) {
      QString ipName = QString::fromStdString(instance->IPName());
      QString moduleName = QString::fromStdString(instance->ModuleName());

      QTreeWidgetItem *itemf = new QTreeWidgetItem(ipParentItem);
      itemf->setText(0, moduleName);
      itemf->setData(0, Qt::UserRole, ipName);
      itemf->setData(0, Qt::WhatsThisPropertyRole, SRC_TREE_IP_FILE_ITEM);
      itemf->setData(0, SetFileDataRole, ipName);

      iFileSum += 1;
    }
  }
  topitemIpInstances->setText(
      0, tr("IP Instances") + QString("(%1)").arg(iFileSum));
}

QTreeWidgetItem *SourcesForm::CreateFolderHierachyTree(QTreeWidgetItem *topItem,
                                                       const QString &path) {
  if (path.isEmpty()) return nullptr;

  int index = path.indexOf(QDir::separator());
  if (index == 0) {  // skip first sepertor
    index = path.indexOf(QDir::separator(), 1);
  }
  QString folder = path.mid(0, index);
  QTreeWidgetItem *itemfolder{ChildByText(topItem, folder)};
  if (!itemfolder) {
    itemfolder = new QTreeWidgetItem(topItem);
    itemfolder->setIcon(0, QIcon(":/images/open-file.png"));
    itemfolder->setText(0, folder);
  }
  if (index != -1)
    return CreateFolderHierachyTree(itemfolder,
                                    path.right(path.size() - index - 1));
  return itemfolder;
}

QTreeWidgetItem *SourcesForm::CreateParentFolderItem(QTreeWidgetItem *parent,
                                                     const QString &text) {
  auto item = new QTreeWidgetItem(parent);
  item->setIcon(0, QIcon(":/images/open-file.png"));
  item->setText(0, text);
  item->setToolTip(0, text);
  return item;
}

QTreeWidgetItem *SourcesForm::ChildByText(QTreeWidgetItem *topItem,
                                          const QString &text) {
  QTreeWidgetItem *itemfolder{nullptr};
  for (size_t i = 0; i < topItem->childCount(); i++) {
    if (topItem->child(i)->text(0) == text) {
      return topItem->child(i);
    }
  }
  return nullptr;
}

QString SourcesForm::StripPath(const QString &path) {
  QString p = path;
  if (p.startsWith(QDir::separator())) p.remove(0, 1);
  if (p.endsWith(QDir::separator())) p.remove(p.size() - 1, 1);
  return p;
}

void SourcesForm::showAddFileDialog(GridType gridType) {
  AddFileDialog *addFileDialog = new AddFileDialog(this);
  addFileDialog->setSelected(gridType);
  connect(addFileDialog, SIGNAL(RefreshFiles()), this,
          SLOT(SlotRefreshSourceTree()));
  addFileDialog->exec();
  addFileDialog->close();
  disconnect(addFileDialog, SIGNAL(RefreshFiles()), this,
             SLOT(SlotRefreshSourceTree()));
}
