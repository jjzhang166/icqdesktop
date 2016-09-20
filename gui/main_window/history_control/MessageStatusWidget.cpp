#include "stdafx.h"
#include "MessageStatusWidget.h"

#include "../../utils/utils.h"
#include "MessageItem.h"
#include "MessageStyle.h"
#include "../../theme_settings.h"

namespace Ui
{

    int32_t MessageStatusWidget::getMaxWidth()
    {
        return Utils::scale_value(30);
    }

    MessageStatusWidget::MessageStatusWidget(HistoryControlPageItem *messageItem)
        : QWidget(messageItem)
        , IsMessageBubbleVisible_(true)
        , IsOutgoing_(true)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void MessageStatusWidget::setTime(const int32_t timestamp)
    {
        TimeText_ =
            QDateTime::fromTime_t(timestamp)
                .time()
                .toString("HH:mm");

        QFontMetrics m(MessageStyle::getTimeFont());

        TimeTextSize_ = m.boundingRect(TimeText_).size();
        assert(TimeTextSize_.isValid());

        if (TimeTextSize_.width() != m.width(TimeText_))
            TimeTextSize_.setWidth(m.width(TimeText_));

        updateGeometry();

        update();
    }

    QSize MessageStatusWidget::sizeHint() const
    {
        assert(!TimeTextSize_.isEmpty());

        return TimeTextSize_;
    }

    void MessageStatusWidget::paintEvent(QPaintEvent *)
    {
        QPainter p(this);

        const auto height = sizeHint().height();

        auto cursorX = 0;

        p.setFont(MessageStyle::getTimeFont());
        p.setPen(getTimeColor());

        const auto textBaseline = height;

        assert(!TimeText_.isEmpty());
        p.drawText(cursorX, textBaseline, TimeText_);
    }

    QColor MessageStatusWidget::getTimeColor() const
    {
        auto curTheme = Ui::get_qt_theme_settings()->themeForContact(aimId_);
        if (!curTheme)
        {
            return MessageStyle::getTimeColor();
        }

        if (IsMessageBubbleVisible_)
        {
            if (IsOutgoing_)
            {
                return curTheme->outgoing_bubble_.time_color_;
            }

            return curTheme->incoming_bubble_.time_color_;
        }

        return curTheme->preview_stickers_.time_color_;
    }

    void MessageStatusWidget::setMessageBubbleVisible(const bool _visible)
    {
        IsMessageBubbleVisible_ = _visible;
    }
}