#ifndef SOURCEGRID_H
#define SOURCEGRID_H

#include <QObject>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

namespace FOEDAG {

enum GridType { GT_ADD, GT_SOURCE, GT_CONSTRAINTS, GT_NETLIST, GT_SIM };

typedef struct tagFileData {
  bool m_isFolder;
  QString m_fileType;
  QString m_fileName;
  QString m_filePath;
} FILEDATA;

typedef FILEDATA filedata;

class sourceGrid : public QWidget {
  Q_OBJECT

 public:
  explicit sourceGrid(GridType type, QWidget *parent = nullptr);

  QList<filedata> getTableViewData();

 public slots:
  void AddFiles();
  void AddDirectories();
  void CreateFile();
  void DeleteTableItem();
  void UpTableItem();
  void DownTableItem();
  void TableViewSelectionChanged();

  void AddTableItem(filedata fdata);

 private:
  GridType m_type;
  //  QToolBar* m_toolBar;
  //  QToolButton* m_toolBtnAdd;
  //  QAction* m_actDel;
  //  QAction* m_actUp;
  //  QAction* m_actDown;
  QPushButton *m_btnAddFile;
  QPushButton *m_btnAddDri;
  QPushButton *m_btnCreateFile;
  QPushButton *m_btnDelete;
  QPushButton *m_btnMoveUp;
  QPushButton *m_btnMoveDown;

  QTableView *m_tableViewSrc;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectModel;

  QList<filedata> m_lisFileData;

 private:
  void MoveTableRow(int from, int to);
  bool IsFileDataExit(filedata fdata);
};
}  // namespace FOEDAG
#endif  // SOURCEGRID_H
