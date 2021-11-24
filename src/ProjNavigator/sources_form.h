#ifndef SOURCES_FORM_H
#define SOURCES_FORM_H
#include <QAction>
#include <QTreeWidget>
#include <QWidget>

#include "NewProject/ProjectManager/project_manager.h"
#include "create_design_dialog.h"

#define SRC_TREE_DESIGN_TOP_ITEM "destopitem"
#define SRC_TREE_CONSTR_TOP_ITEM "constrtopitem"
#define SRC_TREE_SIM_TOP_ITEM "simtopitem"
#define SRC_TREE_DESIGN_SET_ITEM "desfilesetitem"
#define SRC_TREE_DESIGN_FILE_ITEM "desfileitem"
#define SRC_TREE_CONSTR_SET_ITEM "constrfilesetitem"
#define SRC_TREE_CONSTR_FILE_ITEM "constrfileitem"
#define SRC_TREE_SIM_SET_ITEM "simfilesetitem"
#define SRC_TREE_SIM_FILE_ITEM "simfileitem"

#define SRC_TREE_FLG_ACTIVE tr(" (Active)")
#define SRC_TREE_FLG_TOP tr(" (Top)")
#define SRC_TREE_FLG_TARGET tr(" (Target)")

namespace Ui {
class SourcesForm;
}

namespace FOEDAG {

class SourcesForm : public QWidget {
  Q_OBJECT

 public:
  explicit SourcesForm(QString strprojpath, QWidget* parent = nullptr);
  ~SourcesForm();

  /*for test*/
  void TestOpenProject(int argc, const char* argv[]);

 private slots:
  void SlotItempressed(QTreeWidgetItem* item, int column);

  void SlotRefreshSourceTree();
  void SlotCreateDesign();
  void SlotAddFile(){};
  void SlotOpenFile(){};
  void SlotRemoveDesign();
  void SlotRemoveFile(){};
  void SlotSetAsTop(){};
  void SlotSetAsTarget(){};
  void SlotSetActive();

 private:
  Ui::SourcesForm* ui;

  QTreeWidget* m_treeSrcHierachy;
  QAction* m_actRefresh;
  QAction* m_actCreateDesign;
  QAction* m_actAddFile;
  QAction* m_actOpenFile;
  QAction* m_actRemoveDesign;
  QAction* m_actRemoveFile;
  QAction* m_actSetAsTop;
  QAction* m_actSetAsTarget;
  QAction* m_actMakeActive;

  ProjectManager* m_projManager;

  void CreateActions();
  void UpdateSrcHierachyTree();
};
}  // namespace FOEDAG

#endif  // SOURCES_FORM_H
