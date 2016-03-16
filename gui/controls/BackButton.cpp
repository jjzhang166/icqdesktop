#include "stdafx.h"
#include "BackButton.h"
#include "../utils/utils.h"

namespace Ui
{
    BackButton::BackButton(QWidget* parent)
        : QPushButton(parent),
        normalColor_(0xf1, 0xf1, 0xf1),
        hoverColor_(0xea, 0xea, 0xea),
        pressedColor_(0xe5, 0xe5, 0xe5),
        current_(UseNormalColor)
    {
		setStyleSheet(Utils::LoadStyle(":/controls/back_button.qss", Utils::get_scale_coefficient(), true));
    }
    BackButton::~BackButton()
    {
        //
    }
    void BackButton::paintEvent(QPaintEvent *event)
    {
        QColor color = normalColor_;
        if (current_ == UseHoverColor)
            color = hoverColor_;
        else if (current_ == UsePressedColor)
            color = pressedColor_;
        
        QPainter painter(this);
        painter.setPen(Qt::PenStyle::NoPen);
        QBrush brush(color);
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        painter.setBrush(brush);
        painter.setRenderHint(QPainter::RenderHint::Antialiasing);
        painter.setRenderHint(QPainter::RenderHint::HighQualityAntialiasing);
        painter.drawEllipse(0, 0, width(), height());
        
        return QPushButton::paintEvent(event);
    }
    bool BackButton::event(QEvent *event)
    {
        if (event->type() == QEvent::Enter)
            (current_ = UseHoverColor), update();
        else if (event->type() == QEvent::Leave)
            (current_ = UseNormalColor), update();
        if (event->type() == QEvent::MouseButtonPress)
            (current_ = UsePressedColor), update();
        
        return QPushButton::event(event);
    }
}
