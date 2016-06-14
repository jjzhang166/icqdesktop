#include "stdafx.h"
#include "SidebarUtils.h"
#include "../../controls/CustomButton.h"
#include "../../utils/utils.h"

namespace Ui
{
    LineWidget::LineWidget(QWidget* parent, int leftMargin, int topMargin, int rightMargin, int bottomMargin)
    {
        auto rootLayout = emptyVLayout(this);
        rootLayout->addSpacerItem(new QSpacerItem(0, topMargin, QSizePolicy::Preferred, QSizePolicy::Fixed));
        {
            auto horLayout = emptyHLayout();
            horLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            line_ = new QWidget(this);
            line_->setStyleSheet("background: #dadada");
            line_->setFixedHeight(Utils::scale_value(1));
            horLayout->addWidget(line_);
            horLayout->addSpacerItem(new QSpacerItem(rightMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
            horLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
            rootLayout->addLayout(horLayout);
        }
        rootLayout->addSpacerItem(new QSpacerItem(0, bottomMargin, QSizePolicy::Preferred, QSizePolicy::Fixed));
    }

    void LineWidget::setLineWidth(int width)
    {
        line_->setFixedWidth(width);
    }

    ActionButton::ActionButton(QWidget* parent, const QString& image, const QString& text, int height, int leftMargin, int textOffset)
        : QWidget(parent)
        , Hovered_(false)
    {
        auto hLayout = emptyHLayout(this);
        hLayout->addSpacerItem(new QSpacerItem(leftMargin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
        Button_ = new CustomButton(this, image);
        Button_->setOffsets(textOffset, 0);
        Button_->setCursor(QCursor(Qt::PointingHandCursor));
        Button_->setText(text);
        Button_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        Button_->setAlign(Qt::AlignLeft);
        Button_->setFocusPolicy(Qt::NoFocus);
        Button_->setFixedHeight(height);
        Button_->setAttribute(Qt::WA_TransparentForMouseEvents);
        hLayout->addWidget(Button_);
        setFixedHeight(height);
    }

    void ActionButton::setImage(const QString& image)
    {
        Button_->setImage(image);
    }

    void ActionButton::setText(const QString& text)
    {
        Button_->setText(text);
    }

    void ActionButton::paintEvent(QPaintEvent* e)
    {
        QWidget::paintEvent(e);
        if (Hovered_)
        {
            QPainter p(this);
            p.fillRect(rect(), QColor(QColor(220, 220, 220, 0.4 * 255)));
        }
    }

    void ActionButton::enterEvent(QEvent* e)
    {
        Hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void ActionButton::leaveEvent(QEvent* e)
    {
        Hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    void ActionButton::mouseReleaseEvent(QMouseEvent* e)
    {
        emit clicked();
        QWidget::mouseReleaseEvent(e);
    }

    ClickedWidget::ClickedWidget(QWidget* parent)
        : QWidget(parent)
        , Hovered_(false)
    {
    }

    void ClickedWidget::paintEvent(QPaintEvent* e)
    {
        QWidget::paintEvent(e);
        if (Hovered_)
        {
            QPainter p(this);
            p.fillRect(rect(), QColor(QColor(220, 220, 220, 0.4 * 255)));
        }
    }

    void ClickedWidget::enterEvent(QEvent* e)
    {
        Hovered_ = true;
        update();
        QWidget::enterEvent(e);
    }

    void ClickedWidget::leaveEvent(QEvent* e)
    {
        Hovered_ = false;
        update();
        QWidget::leaveEvent(e);
    }

    void ClickedWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        emit clicked();
        QWidget::mouseReleaseEvent(e);
    }

    QHBoxLayout* emptyHLayout(QWidget* parent)
    {
        auto layout = new QHBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        return layout;
    }

    QVBoxLayout* emptyVLayout(QWidget* parent)
    {
        auto layout = new QVBoxLayout(parent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        return layout;
    }
}