#pragma once

#include <QObject>

#include "Compiler/Task.h"

namespace FOEDAG {

class TaskStatusWatcher : public QObject {
  Q_OBJECT

 public:
  static TaskStatusWatcher *Instance();

  bool isSynthResultDirty() const { return m_isSynthResultDirty; }

signals:
  void synthResultDirty();
  void synthSucceeded();
  void synthFailed();

public slots:
  void onTaskDone(uint taskId, TaskStatus status);
  void onDesignFilesChanged();

 private:
  bool m_isSynthResultDirty = false;
  bool m_isDesignChangedFirstTime = true; // when initially project is loaded it emit signal about design files change, we want to skip this event
};

}  // namespace FOEDAG
