#include "NCriticalPathStatusBar.h"

#include <QHBoxLayout>
#include <QLabel>

#include "NCriticalPathTheme.h"

NCriticalPathStatusBar::NCriticalPathStatusBar(QWidget* parent)
    : QWidget(parent) {
  QHBoxLayout* layout = new QHBoxLayout;
  int borderSize = NCriticalPathTheme::instance().borderSize();
  layout->setContentsMargins(borderSize, 0, 0, 0);
  setLayout(layout);

  m_lbConnectionStatus = new QLabel;
  int indicatorSize = NCriticalPathTheme::instance().statusIndicatorSize();
  m_lbConnectionStatus->setFixedSize(indicatorSize, indicatorSize);
  layout->addWidget(m_lbConnectionStatus);

  m_lbMessage = new QLabel;
  layout->addWidget(m_lbMessage);

  onConnectionStatusChanged(false);
}

void NCriticalPathStatusBar::onConnectionStatusChanged(bool isConnected) {
  int indicatorSize = NCriticalPathTheme::instance().statusIndicatorSize();
  const QColor& okColor =
      NCriticalPathTheme::instance().statusIndicatorOkColor();
  const QColor& busyColor =
      NCriticalPathTheme::instance().statusIndicatorBusyColor();
  m_lbConnectionStatus->setStyleSheet(
      QString("border: 1px solid black; border-radius: %1px; background: %2;")
          .arg(indicatorSize / 2)
          .arg(isConnected ? okColor.name() : busyColor.name()));
}

void NCriticalPathStatusBar::onMessageChanged(const QString& msg) {
  setMessage(msg);
}

void NCriticalPathStatusBar::setMessage(const QString& msg) {
  m_lbMessage->setText(msg);
}

void NCriticalPathStatusBar::clear() { m_lbMessage->setText(""); }
