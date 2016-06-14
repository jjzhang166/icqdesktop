#include "stdafx.h"
#include "FlatMenu.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"

namespace Ui
{
    int FlatMenuStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
    {
        int s = QProxyStyle::pixelMetric(metric, option, widget);
        if (metric == QStyle::PM_SmallIconSize)
        {
            s = Utils::scale_value(24);
        }
        return s;
    }

    FlatMenu::FlatMenu(QWidget* parent/* = nullptr*/):
        QMenu(parent), iconSticked_(false)
    {
        setWindowFlags(windowFlags() | Qt::NoDropShadowWindowHint);
        setStyle(new FlatMenuStyle());
        setStyleSheet(QString("QMenu {\
                       background-color: #f2f2f2;\
                       border: 1px solid #cccccc;\
                       }\
                       QMenu::item {\
                       background-color: transparent;\
                       color: #000000;\
                       height: %1px;\
                       }\
                       QMenu::item:selected\
                       {\
                       background-color: #e2e2e2;\
                       height: %1px;\
                       }").arg(Utils::scale_value(40)));

        QFont font = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16));
        setFont(font);
        if (platform::is_apple())
        {
            QWidget::connect(&Utils::InterConnector::instance(), SIGNAL(closeAnyPopupWindow()), this, SLOT(close()));
        }
    }

    FlatMenu::~FlatMenu()
    {
        //
    }

    void FlatMenu::paintEvent(QPaintEvent *event)
    {
        QMenu::paintEvent(event);
        QPainter p(this);
        p.setClipRect(0, 0, geometry().width(), geometry().height());
        setMask(p.clipRegion());
    }

    void FlatMenu::hideEvent(QHideEvent* event)
    {
        QMenu::hideEvent(event);
    }

    void FlatMenu::showEvent(QShowEvent* event)
    {
        if (!parentWidget())
        {
            return QMenu::showEvent(event);
        }
        auto posx = pos().x();
        auto posy = pos().y();
        if (iconSticked_)
        {
            if (auto button = qobject_cast<QPushButton *>(parentWidget()))
            {
                posx -= ((button->size().width() - button->iconSize().width()) / 2);
                posy -= ((button->size().height() - button->iconSize().height()) / 2);
                move(posx, posy);
            }
        }
        if ((expandDirection_ & Qt::AlignLeft) == Qt::AlignLeft)
        {
            move(pos().x() + parentWidget()->geometry().width() - geometry().width(), pos().y());
        }
        if ((expandDirection_ & Qt::AlignTop) == Qt::AlignTop)
        {
            move(pos().x(), pos().y() - parentWidget()->geometry().height() - geometry().height());
        }
    }

    void FlatMenu::setExpandDirection(Qt::Alignment direction)
    {
        expandDirection_ = direction;
    }
    
    void FlatMenu::stickToIcon()
    {
        iconSticked_ = true;
    }
}