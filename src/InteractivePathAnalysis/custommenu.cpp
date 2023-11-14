#include "custommenu.h"

#include <QPushButton>
#include <QDebug>

CustomMenu::CustomMenu(QPushButton* caller): QWidget(caller)
{
    setWindowFlags(Qt::Popup);

    connect(caller, &QPushButton::clicked, this, [this](){
        if (!isVisible()) {
            QWidget* parentWidget = qobject_cast<QWidget*>(parent());
            QPoint newPos = parentWidget->mapToGlobal(QPoint(0,0)); // global coord system because of popup property
            newPos.setY(newPos.y() + parentWidget->height());
            popup(newPos);
        }
    });

    hide(); // initially hide
}

void CustomMenu::popup(QPoint pos)
{
    show(); // show first before move, otherwise on first run we will have not proper widget size due to not initilized geometry
    switch(m_alignment) {
    case Alignment::LEFT: /*do nothing*/ break;
    case Alignment::RIGHT: pos.setX(pos.x() - width()); break;
    case Alignment::MIDDLE: pos.setX(pos.x() - 0.5*width()); break;
    }
    move(pos);
}

void CustomMenu::mousePressEvent(QMouseEvent* event)
{
    if (!rect().contains(event->pos())) {
        hide();
    }
    QWidget::mousePressEvent(event);
}

void CustomMenu::hideEvent(QHideEvent* event)
{
    emit hidden();
    QWidget::hideEvent(event);
}
