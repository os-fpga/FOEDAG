#include "CustomMenu.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

CustomMenu::CustomMenu(QPushButton* caller) : QWidget(caller) {
  setWindowFlags(Qt::Popup);

  QVBoxLayout* layout = new QVBoxLayout;
  QWidget::setLayout(layout);

  m_contentLayout = new QVBoxLayout;
  m_contentLayout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(m_contentLayout);

  QHBoxLayout* buttonsLayout = new QHBoxLayout;
  layout->addLayout(buttonsLayout);

  m_bnCancel = new QPushButton(tr("Cancel"));
  m_bnDone = new QPushButton(tr("Done"));

  connect(m_bnCancel, &QPushButton::clicked, this, [this]() {
    hide();
  });
  connect(m_bnDone, &QPushButton::clicked, this, [this]() {
    m_isAccepted = true;
    hide();
    emit accepted();
  });

  buttonsLayout->addStretch(1);
  buttonsLayout->addWidget(m_bnCancel);
  buttonsLayout->addWidget(m_bnDone);
  buttonsLayout->addStretch(1);

  connect(caller, &QPushButton::clicked, this, [this]() {
    if (!isVisible()) {
      QWidget* parentWidget = qobject_cast<QWidget*>(parent());
      QPoint newPos = parentWidget->mapToGlobal(
          QPoint(0, 0));  // global coord system because of popup property
      newPos.setY(newPos.y() + parentWidget->height());
      popup(newPos);
    }
  });

  hide();  // initially hide
}

void CustomMenu::setButtonToolTips(const QString& toolTipForDoneButton,
                                   const QString& toolTipForCancelButton) {
  m_bnDone->setToolTip(toolTipForDoneButton);
  m_bnCancel->setToolTip(toolTipForCancelButton);
}

void CustomMenu::addContentLayout(QLayout* layout) {
  m_contentLayout->addLayout(layout);
}

void CustomMenu::popup(QPoint pos) {
  m_isAccepted = false;
  QWidget::show();  // show first before move, otherwise on first run we will
                    // have not proper widget size due to not initilized
                    // geometry
  switch (m_alignment) {
    case Alignment::LEFT: /*do nothing*/
      break;
    case Alignment::RIGHT:
      pos.setX(pos.x() - width());
      break;
    case Alignment::MIDDLE:
      pos.setX(pos.x() - 0.5 * width());
      break;
  }
  move(pos);
}

void CustomMenu::mousePressEvent(QMouseEvent* event) {
  if (!rect().contains(event->pos())) {
    hide();
  }
  QWidget::mousePressEvent(event);
}

void CustomMenu::hideEvent(QHideEvent* event) {
  if (!m_isAccepted) {
    emit declined();
  }
  QWidget::hideEvent(event);
}
