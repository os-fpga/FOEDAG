// sma find where to get current copyright text from

#ifndef TASKS_H
#define TASKS_H

namespace FOEDAG {

class Tasks {
 private:
  Tasks();

  static Tasks* m_instance;

 public:
  static Tasks* getInstance();
  static void getTasks();
};

}  // namespace FOEDAG

#endif