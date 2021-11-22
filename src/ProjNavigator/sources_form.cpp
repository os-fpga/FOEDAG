#include "sources_form.h"

#include "NewProject/ProjectManager/project.h"
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
  m_actAddFileSet = new QAction(tr("Add File Sets"), m_treeSrcHierachy);
  m_actAddSrc = new QAction(tr("Add Sources"), m_treeSrcHierachy);
  m_actOpenFile = new QAction(tr("Open File"), m_treeSrcHierachy);
  m_actRemoveFileSet = new QAction(tr("Remove File Set"));
  m_actRemoveFile = new QAction(tr("Remove File"), m_treeSrcHierachy);
  m_actSetAsTop = new QAction(tr("Set As Top"), m_treeSrcHierachy);
  m_actSetAsTarget =
      new QAction(tr("Set as Target Constraint File"), m_treeSrcHierachy);
  m_actMakeActive = new QAction(tr("Make Active"), m_treeSrcHierachy);

  // connect(m_treeViewSrc, SIGNAL(itemPressed(QTreeWidgetItem*,int)), this,
  // SLOT(slot_hitempressedSlot(QTreeWidgetItem*,int)));

  ui->m_tabWidget->removeTab(ui->m_tabWidget->indexOf(ui->tab_2));
}

SourcesForm::~SourcesForm() { delete ui; }

void SourcesForm::UpdateSrcHierachyTree() {
  m_treeSrcHierachy->clear();

  QTreeWidgetItem *topitemDS = new QTreeWidgetItem(m_treeSrcHierachy);
  topitemDS->setText(0, tr("Design Sources (0)"));
  topitemDS->setData(0, Qt::WhatsThisPropertyRole, QString("topitem"));
  m_treeSrcHierachy->addTopLevelItem(topitemDS);

  QTreeWidgetItem *topitemCS = new QTreeWidgetItem(m_treeSrcHierachy);
  m_treeSrcHierachy->addTopLevelItem(topitemCS);
  topitemCS->setText(0, tr("Constraints (0)"));
  topitemCS->setData(0, Qt::WhatsThisPropertyRole, QString("topitem"));

  QTreeWidgetItem *topitemSS = new QTreeWidgetItem(m_treeSrcHierachy);
  m_treeSrcHierachy->addTopLevelItem(topitemSS);

  topitemSS->setText(0, tr("Simulation Sources (0)"));
  topitemSS->setData(0, Qt::WhatsThisPropertyRole, QString("topitem"));

  //隐藏表头
  m_treeSrcHierachy->setHeaderHidden(true);
  //设置展开
  m_treeSrcHierachy->expandAll();
}
