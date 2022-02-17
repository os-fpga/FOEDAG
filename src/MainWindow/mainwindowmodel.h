#ifndef MAINWINDOWMODEL_H
#define MAINWINDOWMODEL_H

#include <QObject>

namespace FOEDAG {

class TclInterpreter;

class MainWindowModel : public QObject {
  Q_OBJECT

  Q_PROPERTY(
      bool visible READ isVisible WRITE setIsVisible NOTIFY visibleChanged)
  Q_PROPERTY(QString statusBarMessage READ statusBarMessage WRITE
                 setStatusBarMessage NOTIFY statusBarMessageChanged)
 public:
  explicit MainWindowModel(TclInterpreter* interp, QObject* parent = nullptr);

  void Tcl_NewProject(int argc, const char* argv[]);
  Q_INVOKABLE QStringList fileDialogFilters();

  bool isVisible() const;
  void setIsVisible(bool newIsVisible);

  const QString& statusBarMessage() const;
  void setStatusBarMessage(const QString& newStatusBarMessage);

 private slots:
  /**
   * @brief create new file with given name
   * @param fileName name of new file
   * @return bool true - if failed, false - if success
   */
  bool createNewFile(const QUrl& fileName);
  //  void newProjectDlg();
  //  void openProject();

 signals:
  void visibleChanged();
  void statusBarMessageChanged();

 private:
  TclInterpreter* m_interpreter;
  bool m_isVisible{true};
  QString m_statusBarMessage;
};
}  // namespace FOEDAG

#endif  // MAINWINDOWMODEL_H
