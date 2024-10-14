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
   * @brief page caption for all page of New project dialog
   * @return Caption depending on the page index
   */
  QString pageHeadCaption(const int index);  // all pages

  /**
   * @brief main text for all page of New project dialog
   * @return text for page depending on the page index
   */
  QString pageMainText(const int index);  // all pages

  /**
   * @brief Label caption. Project name
   * @return Label caption
   */
  QString projectNameCaption();  // 0 page

  /**
   * @brief Label caption. Project location
   * @return Label caption
   */
  QString projectLocationCaption();  // 0 page

  /**
   * @brief Label caption. Checkbox
   * @return Label caption
   */
  QString checkBoxSubDirectoryCaption();  // 0 page

  /**
   * @brief Label caption. Project full path
   * @return Label caption
   */
  QString projectFullPathCaption();  // 0 page

  /**
   * @brief radio button caption. RTL Project
   * @return radio button caption
   */
  QString radioButtonRTLProjectCaption();  // 1 page

  /**
   * @brief text under RTL Project radio button
   * @return text
   */
  QString textRTLProject();  // 1 page

  /**
   * @brief radio button caption. Post-synthesis Project
   * @return radio button caption
   */
  QString radioButtonPostSynthesisProjectCaption();  // 1 page

  /**
   * @brief text under PostSynthesis Project radio button
   * @return text
   */
  QString textPostSynthesisProject();  // 1 page

  /**
   * @brief radio button caption. Post-synthesis Project
   * @return radio button caption
   */
  QString radioButtonSynplifyProjectCaption();  // 1 page

  /**
   * @brief text under PostSynthesis Project radio button
   * @return text
   */
  QString textSynplifyProject();  // 1 page

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

  const QString &projectType() const;
  void setProjectType(const QString &newProjectType);

 signals:
  void projectNameChanged();
  void projectLocationChanged();
  void needToCreateProjrctSubDirectoryChanged();
  void projectTypeChanged();

 protected:
  QString m_projectName{"project_1"};
  QString m_projectLocation;
  QString m_projectType{"RTL"};  // QString "RTL" or "Post-synthesis" or "Synplify"
  bool m_needToCreateProjrctSubDirectory{false};
};

}  // namespace FOEDAG

#endif  // NEWPROJECTMODEL_H
