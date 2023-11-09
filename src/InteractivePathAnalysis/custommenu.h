#pragma once

#include <QWidget>

#include <QPushButton>
#include <QMouseEvent>
#include <QDebug>

class CustomMenu final : public QWidget
{
    Q_OBJECT
public:
    enum Alignment {
        LEFT,
        RIGHT,
        MIDDLE
    };

    CustomMenu(QPushButton* caller);

    void setAlignment(Alignment alignment) { m_alignment = alignment; }
    void popup(QPoint pos);

signals:
    void hidden();

protected:
    void mousePressEvent(QMouseEvent* event) override final;
    void hideEvent(QHideEvent* event) override final;

private:
    Alignment m_alignment = Alignment::LEFT;
};
