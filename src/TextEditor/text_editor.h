#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QObject>

#include "text_editor_form.h"

namespace FOEDAG {

class TextEditor : public QObject {
  Q_OBJECT
 public:
  explicit TextEditor(QObject *parent = nullptr);

  void ShowTextEditor();
  QWidget *GetTextEditor();

 public slots:
  void OpenFile(const QString &strFileName);
};
}  // namespace FOEDAG
#endif  // TEXTEDITOR_H
