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

typedef struct tagFileData {
  bool m_isFolder;
  QString m_fileType;
  QString m_fileName;
  QString m_filePath;
} FILEDATA;

typedef FILEDATA filedata;

class RunsGrid : public QWidget {
  Q_OBJECT
 public:
  explicit RunsGrid(RunsType type, QWidget *parent = nullptr);

 signals:

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
