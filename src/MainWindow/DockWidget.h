#pragma once

#include <QDockWidget>

namespace FOEDAG {

class DockWidget : public QDockWidget {
    Q_OBJECT

public:
    DockWidget(const QString &title, QWidget *parent = nullptr)
        : QDockWidget(title, parent) {}

signals:
    void closed();

protected:
    void closeEvent(QCloseEvent *event) override final {
        QDockWidget::closeEvent(event);
        emit closed();
    }
};

}  // namespace FOEDAG
