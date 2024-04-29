#pragma once

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <QEasingCurve>

class RoundProgressWidget : public QWidget
{
    Q_OBJECT
    
    const int ANIMATION_INTERVAL_MS = 20;
    const qreal PROGRESS_STEP = ANIMATION_INTERVAL_MS * 0.001;

public:
    RoundProgressWidget(int size, QWidget* parent = nullptr);
    ~RoundProgressWidget()=default;

protected:
    void showEvent(QShowEvent*) override final;
    void hideEvent(QHideEvent*) override final;
    void paintEvent(QPaintEvent*) override final;

private:
    QTimer m_timer;
    QPixmap m_pixmap;
    qreal m_t = 0.0;

    QEasingCurve m_animCurve;
};

