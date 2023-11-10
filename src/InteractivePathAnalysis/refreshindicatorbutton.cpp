#include "refreshindicatorbutton.h"

RefreshIndicatorButton::RefreshIndicatorButton(const QString& name, QWidget* parent)
    : QPushButton(parent), m_origText(name)
{
    markDirty();
}

void RefreshIndicatorButton::clearDirty()
{
    m_isDirty = false;
    updateText();
}

void RefreshIndicatorButton::markDirty()
{
    m_isDirty = true;
    updateText();
}

void RefreshIndicatorButton::updateText()
{
    QString text = m_origText;
    if (m_isDirty) {
        text += "*";
    }
    setText(text);
}
