#ifndef MAINWINDOWMODEL_H
#define MAINWINDOWMODEL_H

#include <qqml.h>

#include <QObject>
#include <memory>

#include "NewFile/newfilemodel.h"
#include "NewProject/newprojectmodel.h"

namespace FOEDAG {

class TclInterpreter;

class MainWindowModel : public QObject {
  Q_OBJECT

  Q_PROPERTY(
      bool visible READ isVisible WRITE setIsVisible NOTIFY visibleChanged)
  Q_PROPERTY(QString statusBarMessage READ statusBarMessage WRITE
                 setStatusBarMessage NOTIFY statusBarMessageChanged)

  Q_PROPERTY(QString projectName READ projectName WRITE setProjectName NOTIFY
                 projectNameChanged)
  Q_PROPERTY(QString projectLocation READ projectLocation WRITE
                 setProjectLocation NOTIFY projectLocationChanged)
  Q_PROPERTY(
      bool needToCreateProjrctSubDirectory READ needToCreateProjrctSubDirectory
          WRITE setNeedToCreateProjrctSubDirectory NOTIFY
              needToCreateProjrctSubDirectoryChanged)

 public:
  explicit MainWindowModel(TclInterpreter* interp, QObject* parent = nullptr);

  /**
   * @brief TODO:
   * @param argc TODO:
   * @param argv TODO:
   */
  void Tcl_NewProject(int argc, const char* argv[]);

  /**
   * @brief set of filters for new file dialog
   * @return list of file filters
   */
  Q_INVOKABLE QStringList newFileDialogFilters();

  /**
   * @brief set of filters for open file dialog
   * @return list of file filters
   */
  Q_INVOKABLE QStringList openFileDialogFilters();

  /**
   * @brief page caption for all page of New projetc dialog
   * @return Caption depending on the page index
   */
  Q_INVOKABLE QString pageHeadCaption(const int index);

  /**
   * @brief main text on first page of New project dialog (location)
   * @return text for first page of New project dialog (location)
   */
  Q_INVOKABLE QString locationPageMainText();

  /**
   * @brief Label caption. Project name
   * @return Label caption
   */
  Q_INVOKABLE QString projectNameCaption();

  /**
   * @brief Label caption. Project location
   * @return Label caption
   */
  Q_INVOKABLE QString projectLocationCaption();

  /**
   * @brief Label caption. Checkbox
   * @return Label caption
   */
  Q_INVOKABLE QString checkBoxSubDirectoryCaption();

  /**
   * @brief Label caption. Project full path
   * @return Label caption
   */
  Q_INVOKABLE QString projectFullPathCaption();

  /**
   * @brief new project full path
   * @return full path that consist of directory and project name
   */
  Q_INVOKABLE QString fullPathToProject();

  const QString& projectName() const;
  void setProjectName(const QString& newProjectName);

  const QString& projectLocation() const;
  void setProjectLocation(const QString& newProjectLocation);

  bool needToCreateProjrctSubDirectory() const;
  void setNeedToCreateProjrctSubDirectory(
      bool newNeedToCreateProjrctSubDirectory);

  // common
  bool isVisible() const;
  void setIsVisible(bool newIsVisible);

  const QString& statusBarMessage() const;
  void setStatusBarMessage(const QString& newStatusBarMessage);

 public slots:
  /**
   * @brief create new file with given name
   * @param fileName name of new file
   * @return bool true - if success, false - if failed
   */
  bool createNewFile(const QUrl& fileName, const QString& extension);
  //  void newProjectDlg();
  //  void openProject();

 signals:
  // common
  void visibleChanged();
  void statusBarMessageChanged();
  // new project dialog
  void projectNameChanged();
  void projectLocationChanged();
  void needToCreateProjrctSubDirectoryChanged();

 private:
  TclInterpreter* m_interpreter;
  bool m_isVisible{true};
  QString m_statusBarMessage;
  std::unique_ptr<NewFileModel> m_newFileModel;
  std::unique_ptr<NewProjectModel> m_newProjectModel;
};
}  // namespace FOEDAG

#endif  // MAINWINDOWMODEL_H
