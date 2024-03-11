#pragma once

#include <QWidget>

#include <QMouseEvent>

class QPushButton;
class QVBoxLayout;

namespace FOEDAG {

/**
 * @brief Simple Menu Implementation
 * 
 * Behaves like a regular menu but allows any type of widgets in the menu body.
 */
class CustomMenu final : public QWidget
{
    Q_OBJECT
public:
    explicit CustomMenu(QPushButton* caller);

    void addContentLayout(QLayout*);
    void setLayout(QLayout* layout)=delete;

    void setAlignment(Qt::Alignment alignment) { m_alignment = alignment; }
    void show()=delete;
    void popup(QPoint pos);

    void setButtonToolTips(const QString& toolTipForDoneButton, const QString& toolTipForCancelButton);

signals:
    void accepted();
    void declined();

protected:
    void mousePressEvent(QMouseEvent* event) override final;
    void hideEvent(QHideEvent* event) override final;

private:
    bool m_isAccepted = false;
    Qt::Alignment m_alignment = Qt::AlignLeft;
    QVBoxLayout* m_contentLayout = nullptr;

    QPushButton* m_bnCancel = nullptr;
    QPushButton* m_bnDone = nullptr;
};

} // namespace FOEDAG