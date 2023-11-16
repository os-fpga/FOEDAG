#include "custommenu.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>

CustomMenu::CustomMenu(QPushButton* caller): QWidget(caller)
{
    setWindowFlags(Qt::Popup);

    QVBoxLayout* layout = new QVBoxLayout;
    QWidget::setLayout(layout);

    m_contentLayout = new QVBoxLayout;
    m_contentLayout->setContentsMargins(0,0,0,0);
    layout->addLayout(m_contentLayout);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    layout->addLayout(buttonsLayout);

    QPushButton* bnCancel = new QPushButton(tr("Cancel"));
    QPushButton* bnApply = new QPushButton(tr("Apply"));

    connect(bnCancel, &QPushButton::clicked, this, [this](){
        hide();

    });
    connect(bnApply, &QPushButton::clicked, this, [this](){
        m_isAccepted = true;
        hide();
        emit accepted();
    });

    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(bnCancel);
    buttonsLayout->addWidget(bnApply);
    buttonsLayout->addStretch(1);

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

void CustomMenu::addContentLayout(QLayout* layout)
{
    m_contentLayout->addLayout(layout);
}

void CustomMenu::popup(QPoint pos)
{
    m_isAccepted = false;
    QWidget::show(); // show first before move, otherwise on first run we will have not proper widget size due to not initilized geometry
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
    if (!m_isAccepted) {
        emit declined();
    }
    QWidget::hideEvent(event);
}
