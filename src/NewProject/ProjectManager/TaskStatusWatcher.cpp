#include "TaskStatusWatcher.h"
#include "Compiler/CompilerDefines.h"

#include <QDebug>

namespace FOEDAG {

TaskStatusWatcher* TaskStatusWatcher::Instance() {
  static TaskStatusWatcher watcher{};
  return &watcher;
}

void TaskStatusWatcher::onTaskDone(uint taskId, TaskStatus status)
{
  if (taskId == SYNTHESIS) {
    if (status == TaskStatus::Success) {
      m_isSynthResultDirty = false;
      emit synthSuccessed();
    } else if (status == TaskStatus::Fail) {
      emit synthFailed();
    }
  }
}

void TaskStatusWatcher::onDesignFilesChanged()
{
  if (!m_isDesignChangedFirstTime) {
    m_isSynthResultDirty = true;
    emit synthResultDirty();
  } else {
    m_isDesignChangedFirstTime = false;
  }
}

}  // namespace FOEDAG
