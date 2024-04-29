#include "RoundProgressWidget.h"

#include <QPainter>

RoundProgressWidget::RoundProgressWidget(int size, QWidget* parent)
: QWidget(parent)
, m_pixmap(QPixmap(":/loading.png").scaled(QSize(size, size), Qt::KeepAspectRatio))
, m_animCurve(QEasingCurve::InOutQuad)
{
    m_timer.setInterval(ANIMATION_INTERVAL_MS);

    connect(&m_timer, &QTimer::timeout, this, [this](){
        m_animProgressNorm += ANIM_PROGRESS_STEP_NORM;
        if (m_animProgressNorm > ANIM_PROGRESS_END_NORM) {
            resetAnimationProgress();
        }

        update();
    });

    resize(size, size);
}

void RoundProgressWidget::showEvent(QShowEvent* event)
{
    resetAnimationProgress();
    m_timer.start();

    QWidget::showEvent(event);
}

void RoundProgressWidget::hideEvent(QHideEvent* event)
{
    m_timer.stop();

    QWidget::hideEvent(event);
}

void RoundProgressWidget::paintEvent(QPaintEvent* event) 
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const float angleDegrees = ROTATION_DEGREES_MAX * m_animCurve.valueForProgress(m_animProgressNorm);
    painter.translate(width() / 2, height() / 2);                       // Translate to the center of the widget
    painter.rotate(angleDegrees);                                       // Rotate around the center
    painter.translate(-m_pixmap.width() / 2, -m_pixmap.height() / 2);   // Translate back to the top-left corner
    painter.drawPixmap(0, 0, m_pixmap); 
}

void RoundProgressWidget::resetAnimationProgress()
{
    m_animProgressNorm = ANIM_PROGRESS_START_NORM;
}