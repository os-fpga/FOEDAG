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
class ProjectManager;
constexpr int NO_PROJECT_TYPE{-1};

class sourceGrid : public QWidget {
  Q_OBJECT

 public:
  explicit sourceGrid(QWidget *parent = nullptr);
  void setProjectType(int projectType);
  int projectType() const;

  void setGridType(GridType type);
  GridType gridType() const;
  QList<filedata> getTableViewData();

  void currentFileSet(const QString &fileSet);
  void selectRow(int row);
  void ClearTable();
  bool AddTableItem(const filedata &fdata);

 public slots:
  void AddFiles();
  void AddDirectories();
  void CreateFile();
  void DeleteTableItem();
  void UpTableItem();
  void DownTableItem();
  void TableViewSelectionChanged();

  void CreateNewFile(FOEDAG::filedata fdata);

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
  ProjectManager *m_projectManager{nullptr};
  int m_projectType{NO_PROJECT_TYPE};
  static const QStringList uniqueExtentions;

 private:
  QStringList GetAllDesignSourceExtentions(int projectType) const;
  void initLanguageCombo(int row, const QVariant &data);
  int CurrentProjectType() const;

  void MoveTableRow(int from, int to);
  bool IsFileDataExist(const filedata &fdata);
  static QComboBox *CreateLanguageCombo(int projectType, GridType gType);
  bool CheckPinFileExists(const QString &suffix);
  bool CheckNetlistFileExists(const QStringList &files);
  bool isPinFileAdded() const;
  bool isNetlistFileAdded() const;
  QString Filter(int projectType, GridType gType) const;
  void VerifyFilesWithSameName(const QStringList &files);
};
}  // namespace FOEDAG

QDebug operator<<(QDebug debug, const FOEDAG::filedata &a);

#endif  // SOURCEGRID_H
