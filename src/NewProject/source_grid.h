#ifndef SOURCEGRID_H
#define SOURCEGRID_H

#include <QObject>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

class QComboBox;

namespace FOEDAG {

enum GridType { GT_ADD, GT_SOURCE, GT_CONSTRAINTS, GT_NETLIST, GT_SIM };

typedef struct tagFileData {
  bool m_isFolder;
  QString m_fileType;
  QString m_fileName;
  int m_language;
  QString m_filePath;
  QString m_workLibrary;
  QString m_groupName;
} FILEDATA;

typedef FILEDATA filedata;

class sourceGrid : public QWidget {
  Q_OBJECT

 public:
  explicit sourceGrid(QWidget *parent = nullptr);

  void setGridType(GridType type);
  QList<filedata> getTableViewData();

  void currentFileSet(const QString &fileSet);
  void selectRow(int row);
  void ClearTable();

 public slots:
  void AddFiles();
  void AddDirectories();
  void CreateFile();
  void DeleteTableItem();
  void UpTableItem();
  void DownTableItem();
  void TableViewSelectionChanged();

  void AddTableItem(FOEDAG::filedata fdata);

 private slots:
  void onItemChanged(QStandardItem *item);
  void languageHasChanged();

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
  void initLanguageCombo(int row, const QVariant &data);

 private:
  void MoveTableRow(int from, int to);
  bool IsFileDataExit(filedata fdata);
  static QComboBox *CreateLanguageCombo();
};
}  // namespace FOEDAG

QDebug operator<<(QDebug debug, const FOEDAG::filedata &a);

#endif  // SOURCEGRID_H
