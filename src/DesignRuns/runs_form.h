#ifndef RUNSFORM_H
#define RUNSFORM_H

#include <QAction>
#include <QApplication>
#include <QObject>
#include <QTreeWidget>
#include <QWidget>

#include "NewProject/ProjectManager/project_manager.h"

#define RUNS_TREE_STATUS "Not Started"
#define RUNS_TREE_ACTIVE "(Active)"

namespace FOEDAG {

class RunsForm : public QWidget {
  Q_OBJECT
 public:
  explicit RunsForm(QString strProPath, QWidget* parent = nullptr);

 private slots:
  void SlotItempressed(QTreeWidgetItem* item, int column);

  void SlotDelete();
  void SlotMakeActive();
  void SlotLaunchRuns();
  void SlotReSetRuns();
  void SlotCreateSynthRuns();
  void SlotCreateImpleRuns();

 signals:

 private:
  QTreeWidget* m_treeRuns;

  QAction* m_actDelete;
  QAction* m_actMakeActive;
  QAction* m_actLaunchRuns;
  QAction* m_actResetRuns;
  QAction* m_actCreateSynthRuns;
  QAction* m_actCreateImpleRuns;

  ProjectManager* m_projManager;

  void CreateActions();
  void UpdateDesignRunsTree();
  void CreateRuns(int type);
};
}  // namespace FOEDAG
#endif  // RUNSFORM_H
