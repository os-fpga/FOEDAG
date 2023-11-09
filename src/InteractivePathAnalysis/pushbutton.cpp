#include "pushbutton.h"

PushButton::PushButton(const QString& name, QWidget* parent)
    : QPushButton(parent), m_origText(name)
{
    markDirty();
}

void PushButton::clearDirty()
{
    m_isDirty = false;
    updateText();
}

void PushButton::markDirty()
{
    m_isDirty = true;
    updateText();
}

void PushButton::updateText()
{
    QString text = m_origText;
    if (m_isDirty) {
        text += "*";
    }
    setText(text);
}
