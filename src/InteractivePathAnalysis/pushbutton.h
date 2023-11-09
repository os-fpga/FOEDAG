#pragma once

#include <QPushButton>

class PushButton : public QPushButton
{
    Q_OBJECT
public:
    PushButton(const QString& name, QWidget* parent = nullptr);

    bool isDirty() const { return m_isDirty; }

    void clearDirty();
    void markDirty();

private:
    bool m_isDirty = false;
    QString m_origText;

    void updateText();
};
