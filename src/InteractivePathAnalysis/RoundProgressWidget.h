#pragma once

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <QEasingCurve>

class RoundProgressWidget : public QWidget
{
    Q_OBJECT
    
    const int ANIMATION_INTERVAL_MS = 20;
    const int ROTATION_DEGREES_MAX = 360;
    const qreal ANIM_PROGRESS_STEP_NORM = ANIMATION_INTERVAL_MS * 0.001;
    const qreal ANIM_PROGRESS_START_NORM = 0.0;
    const qreal ANIM_PROGRESS_END_NORM = 1.0;

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
    qreal m_animProgressNorm = 0.0;

    QEasingCurve m_animCurve;

    void resetAnimationProgress();
};

