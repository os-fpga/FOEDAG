#ifndef MAINWINDOWMODEL_H
#define MAINWINDOWMODEL_H

#include <QObject>

namespace FOEDAG {

class TclInterpreter;

class MainWindowModel : public QObject {
  Q_OBJECT
 public:
  explicit MainWindowModel(TclInterpreter* interp, QObject* parent = nullptr);

  void Tcl_NewProject(int argc, const char* argv[]);

 private slots: /* slots */
                //  void newFile();
                //  void newProjectDlg();
                //  void openProject();

 signals:
 private:
  TclInterpreter* m_interpreter;
};

}  // namespace FOEDAG

#endif  // MAINWINDOWMODEL_H
