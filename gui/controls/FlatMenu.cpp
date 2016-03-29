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
        QMenu(parent)
    {
        setStyle(new FlatMenuStyle());
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
        p.setClipRect(1, 1, geometry().width(), geometry().height());
        setMask(p.clipRegion());
    }

    void FlatMenu::hideEvent(QHideEvent* event)
    {
        QMenu::hideEvent(event);
    }

    void FlatMenu::showEvent(QShowEvent* event)
    {
        if (!parentWidget())
            return QMenu::showEvent(event);
        if ((expandDirection_ & Qt::AlignLeft) == Qt::AlignLeft)
            move(pos().x() + parentWidget()->geometry().width() - geometry().width(), pos().y());
        if ((expandDirection_ & Qt::AlignTop) == Qt::AlignTop)
            move(pos().x(), pos().y() - parentWidget()->geometry().height() - geometry().height());
    }

    void FlatMenu::setExpandDirection(Qt::Alignment direction)
    {
        expandDirection_ = direction;
    }
}