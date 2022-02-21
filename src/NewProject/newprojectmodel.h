#ifndef NEWPROJECTMODEL_H
#define NEWPROJECTMODEL_H

#include <QObject>

namespace FOEDAG {

class NewProjectModel : public QObject {
  Q_OBJECT
 public:
  explicit NewProjectModel(QObject *parent = nullptr);

  // functions for UI
  /**
   * @brief page caption for all page of New projetc dialog
   * @return Caption depending on the page index
   */
  QString pageHeadCaption(const int index);

  /**
   * @brief main text on first page of New project dialog (location)
   * @return text for first page of New project dialog (location)
   */
  QString locationPageMainText();

  /**
   * @brief Label caption. Project name
   * @return Label caption
   */
  QString projectNameCaption();

  /**
   * @brief Label caption. Project location
   * @return Label caption
   */
  QString projectLocationCaption();

  /**
   * @brief Label caption. Checkbox
   * @return Label caption
   */
  QString checkBoxSubDirectoryCaption();

  /**
   * @brief Label caption. Project full path
   * @return Label caption
   */
  QString projectFullPathCaption();

  /**
   * @brief new project full path
   * @return full path that consist of directory and project name
   */
  QString fullPathToProject();

  const QString &projectName() const;
  void setProjectName(const QString &newProjectName);

  const QString &projectLocation() const;
  void setProjectLocation(const QString &newProjectLocation);

  bool needToCreateProjrctSubDirectory() const;
  void setNeedToCreateProjrctSubDirectory(
      bool newNeedToCreateProjrctSubDirectory);

 signals:
  void projectNameChanged();
  void projectLocationChanged();
  void needToCreateProjrctSubDirectoryChanged();

 protected:
  QString m_projectName{"project_1"};
  QString m_projectLocation;
  bool m_needToCreateProjrctSubDirectory{false};
};

}  // namespace FOEDAG

#endif  // NEWPROJECTMODEL_H
