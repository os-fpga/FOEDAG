#ifndef SOURCEGRID_H
#define SOURCEGRID_H

#include <QObject>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#define QSTRING_ST_V "v"
#define QSTRING_ST_VHD "vhd"
#define QSTRING_ST_CDC "CDC"
#define QSTRING_ST_VQM "vqm"
#define QSTRING_ST_HEX "hex"
#define QSTRING_ST_MIF "mif"

enum grid_type { GT_ADD, GT_SOURCE, GT_CONSTRAINTS, GT_NETLIST, GT_SIM };

enum source_type { ST_V, ST_VHD, ST_CDC, ST_VQM, ST_HEX, ST_MIF };

struct filedata {
  int isfolder;     // 0:file 1:folder
  int filetype;     // 0:v 1:vhd 2:CDC 3:vqm 4:hex 5:mif
  QString strtype;  // In fact, Maybe can use string to distinguish file type
  QString fname;
  QString fpath;
};

class sourceGrid : public QWidget {
  Q_OBJECT

  grid_type m_type;
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

  QTableView *m_grid;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectmodel;

  QList<filedata> m_fdatalist;

  void moverow(int from, int to);
  bool isfiledataexit(filedata fdata);

 public:
  explicit sourceGrid(grid_type type, QWidget *parent = nullptr);

  void initsourcegrid(const QList<filedata> flist);
  QList<filedata> getgriddata();
 signals:

 public slots:
  void slot_addfiles();
  void slot_adddirectories();
  void slot_createfile();
  void slot_deletegriditem();
  void slot_upgriditem();
  void slot_downgriditem();

  void slot_selectionChanged();

  void slot_addgridrow(filedata fdata);
};

#endif  // SOURCEGRID_H
