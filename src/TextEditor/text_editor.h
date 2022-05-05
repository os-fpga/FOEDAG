#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QObject>
#include <QWidget>

#include "text_editor_form.h"

namespace FOEDAG {
class Session;

class TextEditor : public QWidget {
  Q_OBJECT
 public:
  explicit TextEditor(QWidget *parent = nullptr);

  void ShowTextEditor();
  void ClosetextEditor();
  QWidget *GetTextEditor();

  void RegisterCommands(FOEDAG::Session *session);

 signals:
  void CurrentFileChanged(QString);

 public slots:
  void SlotOpenFile(const QString &strFileName);
  void SlotOpenFileWithLine(const QString &strFileName, int line);

 private slots:
  void SlotCurrentFileChanged(const QString &strFileName);
};

void registerTextEditorCommands(QWidget *editor, FOEDAG::Session *session);

}  // namespace FOEDAG
#endif  // TEXTEDITOR_H
