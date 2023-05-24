#ifndef SOURCES_FORM_H
#define SOURCES_FORM_H
#include <QAction>
#include <QSettings>
#include <QTreeWidget>
#include <QWidget>

#include "NewProject/ProjectManager/project_manager.h"
#include "add_file_dialog.h"
#include "create_fileset_dialog.h"

#define SRC_TREE_DESIGN_TOP_ITEM "destopitem"
#define SRC_TREE_CONSTR_TOP_ITEM "constrtopitem"
#define SRC_TREE_SIM_TOP_ITEM "simtopitem"
#define SRC_TREE_IP_TOP_ITEM "iptopitem"
#define SRC_TREE_DESIGN_SET_ITEM "desfilesetitem"
#define SRC_TREE_DESIGN_FILE_ITEM "desfileitem"
#define SRC_TREE_CONSTR_SET_ITEM "constrfilesetitem"
#define SRC_TREE_CONSTR_FILE_ITEM "constrfileitem"
#define SRC_TREE_SIM_SET_ITEM "simfilesetitem"
#define SRC_TREE_SIM_FILE_ITEM "simfileitem"
#define SRC_TREE_IP_SET_ITEM "ipfilesetitem"
#define SRC_TREE_IP_INST_ITEM "ipinstitem"
#define SRC_TREE_IP_FILE_ITEM "ipfileitem"

#define SRC_TREE_FLG_ACTIVE tr(" (Active)")
#define SRC_TREE_FLG_TARGET tr(" (Target)")

namespace Ui {
class SourcesForm;
}

namespace FOEDAG {

class TclCommandIntegration;
class SourcesForm : public QWidget {
  Q_OBJECT

 public:
  explicit SourcesForm(QSettings* settings, QWidget* parent = nullptr);
  ~SourcesForm();

  void InitSourcesForm();
  TclCommandIntegration* createTclCommandIntegarion();
  ProjectManager* ProjManager();

  void CreateConstraint();
  void UpdateSrcHierachyTree();
  QAction* ProjectSettingsActions() const;

 signals:
  void OpenFile(QString);
  void OpenFileWith(QString, int);
  void ShowProperty(const QString&);
  void ShowPropertyPanel();
  void CloseProject();
  void IpReconfigRequested(const QString& ipName, const QString moduleName,
                           const QStringList& paramList);
  void IpRemoveRequested(const QString& moduleName);
  void IpDeleteRequested(const QString& moduleName);
  void IpSimulationRequested(const QString& moduleName);
  void IpWaveFormRequest(const QString& moduleName);
  void OpenProjectSettings();

 private slots:
  void SlotItempressed(QTreeWidgetItem* item, int column);
  void SlotItemDoubleClicked(QTreeWidgetItem* item, int column);

  void SetCurrentFileItem(const QString& strFileName);
  void SlotRefreshSourceTree();
  void SlotCreateConstrSet();
  void SlotCreateSimSet();
  void SlotAddFile();
  void SlotOpenFile();
  void SlotOpenFileWith(int editor);
  void SlotRemoveFileSet();
  void SlotRemoveFile();
  void SlotSetAsTarget();
  void SlotSetActive();
  void SlotProperties();
  void SlotPropertiesTriggered();
  void SlotReConfigureIp();
  void SlotRemoveIp();
  void SlotDeleteIp();
  void SlotSimulateIp();
  void SlotWaveForm();

 private:
  Ui::SourcesForm* ui;

  QTreeWidget* m_treeSrcHierachy;
  QAction* m_actRefresh;
  QAction* m_actEditConstrsSets;
  QAction* m_actEditSimulSets;
  QAction* m_actAddFile;
  QAction* m_actOpenFile;
  QAction* m_actRemoveFileset;
  QAction* m_actRemoveFile;
  QAction* m_actSetAsTarget;
  QAction* m_actMakeActive;
  QAction* m_actProperties;
  QAction* m_actCloseProject;
  QAction* m_actReconfigureIp;
  QAction* m_actRemoveIp;
  QAction* m_actDeleteIp;
  QAction* m_simulateIp;
  QAction* m_waveFormView;
  QAction* m_actProjectSettings;
  QMenu* m_menuOpenFileWith;

  ProjectManager* m_projManager;
  QSettings* m_setting{nullptr};

  void CreateActions();
  void CreateFolderHierachyTree();
  static QTreeWidgetItem* CreateFolderHierachyTree(QTreeWidgetItem* topItem,
                                                   const QString& path);
  static QTreeWidgetItem* CreateParentFolderItem(QTreeWidgetItem* parent,
                                                 const QString& text);
  static QTreeWidgetItem* ChildByText(QTreeWidgetItem* topItem,
                                      const QString& text);
  static QString StripPath(const QString& path);
  void showAddFileDialog(GridType gridType);
  void AddIpInstanceTree(QTreeWidgetItem* topItem);
  QStringList SelectedIpModules() const;
  QStringList SelectedFiles() const;
  void initOpenWithMenu(QMenu* menu);
};
}  // namespace FOEDAG

#endif  // SOURCES_FORM_H
