#include "ncriticalpathstatusbar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

NCriticalPathStatusBar::NCriticalPathStatusBar(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(5,0,0,0);
    setLayout(layout);

    m_lbConnectionStatus = new QLabel;
    m_lbConnectionStatus->setFixedSize(10, 10);
    layout->addWidget(m_lbConnectionStatus);

    m_lbMessage = new QLabel;
    layout->addWidget(m_lbMessage);

    onConnectionStatusChanged(false);
}

void NCriticalPathStatusBar::onConnectionStatusChanged(bool isConnected)
{
    m_lbConnectionStatus->setStyleSheet(QString("border: 1px solid black; border-radius: 5px; background: %1;").arg(isConnected? "green": "red"));
}

void NCriticalPathStatusBar::onMessageChanged(const QString& msg)
{
    setMessage(msg);
}

void NCriticalPathStatusBar::setMessage(const QString& msg)
{
    m_lbMessage->setText(msg);
}
