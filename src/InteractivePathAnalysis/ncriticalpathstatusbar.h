#pragma once

#include <QWidget>

class QLabel;

class NCriticalPathStatusBar : public QWidget {
    Q_OBJECT
public:
    NCriticalPathStatusBar(QWidget* parent = nullptr);
    ~NCriticalPathStatusBar()=default;

    void setMessage(const QString& msg);

public slots:
    void onConnectionStatusChanged(bool isConnected);
    void onMessageChanged(const QString& msg);

private:
    QLabel* m_lbConnectionStatus = nullptr;
    QLabel* m_lbMessage = nullptr;
};
