#include "sources_form.h"

#include "NewProject/ProjectManager/project_manager.h"
#include "ui_sources_form.h"

SourcesForm::SourcesForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::SourcesForm) {
  ui->setupUi(this);

  m_treeSrcHierachy = new QTreeWidget(ui->m_tabHierarchy);
  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->addWidget(m_treeSrcHierachy);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);
  ui->m_tabHierarchy->setLayout(vbox);

  UpdateSrcHierachyTree();

  m_actRefresh = new QAction(tr("Refresh Hierarchy"), m_treeSrcHierachy);
  m_actAddFileSet = new QAction(tr("Add FileSet"), m_treeSrcHierachy);
  m_actAddSrc = new QAction(tr("Add Sources"), m_treeSrcHierachy);
  m_actOpenFile = new QAction(tr("Open File"), m_treeSrcHierachy);
  m_actRemoveFileSet = new QAction(tr("Remove FileSet"), m_treeSrcHierachy);
  m_actRemoveFile = new QAction(tr("Remove File"), m_treeSrcHierachy);
  m_actSetAsTop = new QAction(tr("Set As TopModule"), m_treeSrcHierachy);
  m_actSetAsTarget =
      new QAction(tr("Set as Target Constraint File"), m_treeSrcHierachy);
  m_actMakeActive = new QAction(tr("Make Active"), m_treeSrcHierachy);

  // connect(m_treeViewSrc, SIGNAL(itemPressed(QTreeWidgetItem*,int)), this,
  // SLOT(slot_hitempressedSlot(QTreeWidgetItem*,int)));

  ui->m_tabWidget->removeTab(ui->m_tabWidget->indexOf(ui->tab_2));
}

SourcesForm::~SourcesForm() { delete ui; }

void SourcesForm::UpdateSrcHierachyTree() {
  ProjectManager *projManager = new ProjectManager(this);
  if (nullptr == projManager) {
    return;
  }

  m_treeSrcHierachy->clear();
  QTreeWidgetItem *topitemDS = new QTreeWidgetItem(m_treeSrcHierachy);
  topitemDS->setText(0, tr("Design Sources"));
  topitemDS->setData(0, Qt::WhatsThisPropertyRole, QString("topitem"));
  m_treeSrcHierachy->addTopLevelItem(topitemDS);

  QStringList listDesFset = projManager->getDesignFileSets();
  QString strDesAct = projManager->getDesignActiveFileSet();
  foreach (auto str, listDesFset) {
    QTreeWidgetItem *itemfolder = new QTreeWidgetItem(topitemDS);
    if (str == strDesAct) {
      itemfolder->setText(0, str + tr("(Active)"));
    } else {
      itemfolder->setText(0, str);
    }
    itemfolder->setData(0, Qt::WhatsThisPropertyRole,
                        QString("desfilesetitem"));

    QStringList listDesFile = projManager->getDesignFiles(str);
    QString strTop = projManager->getDesignTopModule(str);
    foreach (auto strfile, listDesFile) {
      QString filename =
          strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
      QTreeWidgetItem *itemf = new QTreeWidgetItem(itemfolder);
      if (strfile == strTop) {
        itemf->setText(0, filename + tr("(Top)"));
      } else {
        itemf->setText(0, filename);
      }
      itemf->setData(0, Qt::WhatsThisPropertyRole, QString("desfileitem"));
    }
  }

  QTreeWidgetItem *topitemCS = new QTreeWidgetItem(m_treeSrcHierachy);
  m_treeSrcHierachy->addTopLevelItem(topitemCS);
  topitemCS->setText(0, tr("Constraints"));
  topitemCS->setData(0, Qt::WhatsThisPropertyRole, QString("topitem"));
  QStringList listConstrFset = projManager->getConstrFileSets();
  QString strConstrAct = projManager->getConstrActiveFileSet();
  foreach (auto str, listConstrFset) {
    QTreeWidgetItem *itemfolder = new QTreeWidgetItem(topitemCS);
    if (str == strConstrAct) {
      itemfolder->setText(0, str + tr("(Active)"));
    } else {
      itemfolder->setText(0, str);
    }
    itemfolder->setData(0, Qt::WhatsThisPropertyRole,
                        QString("constrfilesetitem"));

    QStringList listConstrFile = projManager->getConstrFiles(str);
    QString strTarget = projManager->getConstrTargetFile(str);
    foreach (auto strfile, listConstrFile) {
      QString filename =
          strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
      QTreeWidgetItem *itemf = new QTreeWidgetItem(itemfolder);
      if (strfile == strTarget) {
        itemf->setText(0, filename + tr("(Target)"));
      } else {
        itemf->setText(0, filename);
      }
      itemf->setData(0, Qt::WhatsThisPropertyRole, QString("constrfileitem"));
    }
  }

  QTreeWidgetItem *topitemSS = new QTreeWidgetItem(m_treeSrcHierachy);
  m_treeSrcHierachy->addTopLevelItem(topitemSS);
  topitemSS->setText(0, tr("Simulation Sources"));
  topitemSS->setData(0, Qt::WhatsThisPropertyRole, QString("topitem"));
  QStringList listSimFset = projManager->getSimulationFileSets();
  QString strSimAct = projManager->getSimulationActiveFileSet();
  foreach (auto str, listSimFset) {
    QTreeWidgetItem *itemfolder = new QTreeWidgetItem(topitemSS);
    if (str == strSimAct) {
      itemfolder->setText(0, str + tr("(Active)"));
    } else {
      itemfolder->setText(0, str);
    }
    itemfolder->setData(0, Qt::WhatsThisPropertyRole,
                        QString("simfilesetitem"));

    QStringList listSimFile = projManager->getSimulationFiles(str);
    QString strTop = projManager->getSimulationTopModule(str);
    foreach (auto strfile, listSimFile) {
      QString filename =
          strfile.right(strfile.size() - (strfile.lastIndexOf("/") + 1));
      QTreeWidgetItem *itemf = new QTreeWidgetItem(itemfolder);
      if (strfile == strTop) {
        itemf->setText(0, filename + tr("(Top)"));
      } else {
        itemf->setText(0, filename);
      }
      itemf->setData(0, Qt::WhatsThisPropertyRole, QString("simfileitem"));
    }
  }

  m_treeSrcHierachy->setHeaderHidden(true);
  m_treeSrcHierachy->expandAll();
}
