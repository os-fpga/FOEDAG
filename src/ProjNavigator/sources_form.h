#ifndef SOURCES_FORM_H
#define SOURCES_FORM_H
#include <QAction>
#include <QTreeWidget>
#include <QWidget>

#include "NewProject/ProjectManager/project_manager.h"

#define SOURCE_TREE_TOPITEM "topitem"
#define SOURCE_TREE_DESFILESETITEM "desfilesetitem"
#define SOURCE_TREE_DESFILEITEM "desfileitem"
#define SOURCE_TREE_CONSTRFSETITEM "constrfilesetitem"
#define SOURCE_TREE_CONSTRFILEITEM "constrfileitem"
#define SOURCE_TREE_SIMFILESETITEM "simfilesetitem"
#define SOURCE_TREE_SIMFILEITEM "simfileitem"

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
  void SlotAddFileSet(){};
  void SlotAddFile(){};
  void SlotOpenFile(){};
  void SlotRemoveFileSet(){};
  void SlotRemoveFile(){};
  void SlotSetAsTop(){};
  void SlotSetAsTarget(){};
  void SlotSetActive(){};

 private:
  Ui::SourcesForm* ui;

  QTreeWidget* m_treeSrcHierachy;
  QAction* m_actRefresh;
  QAction* m_actAddFileSet;
  QAction* m_actAddFile;
  QAction* m_actOpenFile;
  QAction* m_actRemoveFileSet;
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
