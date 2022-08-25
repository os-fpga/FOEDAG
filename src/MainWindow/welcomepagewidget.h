#ifndef WELCOMEPAGEWIDGET_H
#define WELCOMEPAGEWIDGET_H

#include <QWidget>

class QAction;
class QVBoxLayout;
class QPixmap;
class QPushButton;

namespace FOEDAG {
class WelcomePageWidget final : public QWidget {
 public:
  explicit WelcomePageWidget(const QString &header, const QString &sourcesPath,
                             QWidget *parent = nullptr);
  virtual ~WelcomePageWidget();

  // Adds a new QToolButton, representing given action, to the vertical layout
  void addAction(QAction &act);

 private:
  QPushButton *createActionButton();

  // Reads WelcomeDescription txt file, located in given path. Returns empty
  // string if the file doesn't exist.
  QString getDescription(const QString &sourcesPath) const;

  QVBoxLayout *m_actionsLayout{nullptr};
};
}  // namespace FOEDAG

#endif  // WELCOMEPAGEWIDGET_H
