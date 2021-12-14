#ifndef RUNGRID_H
#define RUNGRID_H

#include <QHeaderView>
#include <QObject>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolBar>
#include <QWidget>

namespace FOEDAG {

enum RunsType { RT_SYNTH, RT_IMPLE };

typedef struct tagRunData {
  int m_iRunType;
  QString m_runName;
  QString m_srcSet;
  QString m_constrSet;
  QString m_device;     // Valid when m_iRunType=RT_SYNTH
  QString m_synthName;  // Valid when m_iRunType=RT_IMPLE
} RUNDATA;

typedef RUNDATA rundata;

class RunsGrid : public QWidget {
  Q_OBJECT
 public:
  explicit RunsGrid(RunsType type, QWidget *parent = nullptr);

 signals:

 private slots:
  void SlotAddRuns();
  void SlotDeleteRuns();

  void SlotTableViewSelectionChanged();

 private:
  RunsType m_type;

  QToolBar *m_toolBar;
  QAction *m_actAdd;
  QAction *m_actDelete;

  QTableView *m_tableViewRuns;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectModel;
};
}  // namespace FOEDAG
#endif  // RUNGRID_H
