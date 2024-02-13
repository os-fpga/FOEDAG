#pragma once

#include <QWidget>

#include <QMouseEvent>

class QPushButton;
class QVBoxLayout;

class CustomMenu final : public QWidget
{
    Q_OBJECT
public:
    enum Alignment {
        LEFT,
        RIGHT,
        MIDDLE
    };

    explicit CustomMenu(QPushButton* caller);

    void addContentLayout(QLayout*);
    void setLayout(QLayout* layout)=delete;

    void setAlignment(Alignment alignment) { m_alignment = alignment; }
    void show()=delete;
    void popup(QPoint pos);

signals:
    void accepted();
    void declined();

protected:
    void mousePressEvent(QMouseEvent* event) override final;
    void hideEvent(QHideEvent* event) override final;

private:
    bool m_isAccepted = false;
    Alignment m_alignment = Alignment::LEFT;
    QVBoxLayout* m_contentLayout = nullptr;
};
