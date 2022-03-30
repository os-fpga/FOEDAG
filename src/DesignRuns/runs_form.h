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

static constexpr uint SaveDataRole = Qt::UserRole + 1;

class RunsForm : public QWidget {
  Q_OBJECT
 public:
  explicit RunsForm(QWidget* parent = nullptr);

  void InitRunsForm(const QString& strFile);
 private slots:
  void SlotItempressed(QTreeWidgetItem* item, int column);

  void SlotRefreshRuns();
  void SlotDelete();
  void SlotMakeActive();
  void SlotLaunchRuns();
  void SlotReSetRuns();
  void SlotCreateRuns();
  void SlotOpenRunDir();

 signals:

 private:
  QTreeWidget* m_treeRuns;

  QAction* m_actDelete;
  QAction* m_actMakeActive;
  QAction* m_actLaunchRuns;
  QAction* m_actResetRuns;
  QAction* m_actCreateRuns;
  QAction* m_actOpenRunDir;

  ProjectManager* m_projManager;

  void CreateActions();
  void UpdateDesignRunsTree();

  void RemoveFolderContent(const QString& strPath);
};
}  // namespace FOEDAG
#endif  // RUNSFORM_H
