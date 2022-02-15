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
  explicit MainWindowModel(QObject* parent = nullptr);
  MainWindowModel(TclInterpreter* interp, QObject* parent = nullptr);

  void Tcl_NewProject(int argc, const char* argv[]);
  Q_INVOKABLE QStringList fileDialogFilters();

  bool isVisible() const;
  void setIsVisible(bool newIsVisible);

  const QString& statusBarMessage() const;
  void setStatusBarMessage(const QString& newStatusBarMessage);

 signals:
  void visibleChanged();
  void statusBarMessageChanged();

 private:
  TclInterpreter* m_interpreter;
  bool m_isVisible{false};
  QString m_statusBarMessage;
};
}  // namespace FOEDAG

#endif  // MAINWINDOWMODEL_H
