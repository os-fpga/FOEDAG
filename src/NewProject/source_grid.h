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
  QString m_workLibrary;
} FILEDATA;

typedef FILEDATA filedata;

class sourceGrid : public QWidget {
  Q_OBJECT

 public:
  explicit sourceGrid(QWidget *parent = nullptr);

  void setGridType(GridType type);
  QList<filedata> getTableViewData();

  void currentFileSet(const QString &fileSet);

 public slots:
  void AddFiles();
  void AddDirectories();
  void CreateFile();
  void DeleteTableItem();
  void UpTableItem();
  void DownTableItem();
  void TableViewSelectionChanged();

  void AddTableItem(filedata fdata);

 private slots:
  void onItemChanged(QStandardItem *item);

 private:
  GridType m_type;
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
  QString m_currentFileSet;
  QStringList GetAllDesignSourceExtentions() const;

 private:
  void MoveTableRow(int from, int to);
  bool IsFileDataExit(filedata fdata);
};
}  // namespace FOEDAG
#endif  // SOURCEGRID_H
