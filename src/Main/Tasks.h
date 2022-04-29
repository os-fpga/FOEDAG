// sma find where to get current copyright text from

#ifndef TASKS_H
#define TASKS_H

#include <QWidget>

namespace FOEDAG {

class Tasks {
 private:
  Tasks();

  static Tasks* m_instance;

 public:
  static Tasks* getInstance();
  static void getTasks();
};

QWidget* createTaskWidgets();  

}  // namespace FOEDAG

#endif