#include "stdafx.h"

#include "../../utils/utils.h"

#include "MessagesScrollbar.h"

namespace
{
    int32_t getPreloadingDistance();
}

namespace Ui
{
    MessagesScrollbar::MessagesScrollbar(QWidget *page)
        : QScrollBar(page)
        , AutoscrollEnablerTimer_(new QTimer(this))
        , CanScrollDown_(false)
    {
        AutoscrollEnablerTimer_->setInterval(5 * 60000);
        AutoscrollEnablerTimer_->setSingleShot(true);

        const auto success = QObject::connect(
            AutoscrollEnablerTimer_, &QTimer::timeout,
            this, &MessagesScrollbar::onAutoScrollTimer
        );
        assert(success);

        setContextMenuPolicy(Qt::NoContextMenu);

        //setStyleSheet("QScrollBar::sub-page:vertical, QScrollBar::add-page:vertical { background: none; }");

        setMaximum(0);
        setPageStep(100);
    }

    bool MessagesScrollbar::canScrollDown() const
    {
        return CanScrollDown_;
    }

    bool MessagesScrollbar::isInFetchRange(const int32_t scrollPos) const
    {
        return (scrollPos < getPreloadingDistance());
    }

    void MessagesScrollbar::scrollToBottom()
    {
        setValue(maximum());
    }

    bool MessagesScrollbar::isAtBottom() const
    {
        return (value() >= maximum());
    }

    void MessagesScrollbar::onAutoScrollTimer()
    {
        CanScrollDown_ = true;
    }
}

namespace
{
    int32_t getPreloadingDistance()
    {
        return Utils::scale_value(1000);
    }
}