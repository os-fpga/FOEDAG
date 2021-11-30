#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QObject>

namespace FOEDAG {

class TextEditor : public QObject {
  Q_OBJECT
 public:
  explicit TextEditor(QObject *parent = nullptr);

 signals:
};
}  // namespace FOEDAG
#endif  // TEXTEDITOR_H
