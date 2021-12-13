#ifndef RUNSFORM_H
#define RUNSFORM_H

#include <QAction>
#include <QObject>
#include <QTreeWidget>
#include <QWidget>

#include "NewProject/ProjectManager/project_manager.h"

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
  void SlotCreateRuns();

 signals:

 private:
  QTreeWidget* m_treeRuns;

  QAction* m_actDelete;
  QAction* m_actMakeActive;
  QAction* m_actLaunchRuns;
  QAction* m_actResetRuns;
  QAction* m_actCreateRuns;

  ProjectManager* m_projManager;

  void CreateActions();
  void UpdateDesignRunsTree();
};
}  // namespace FOEDAG
#endif  // RUNSFORM_H
