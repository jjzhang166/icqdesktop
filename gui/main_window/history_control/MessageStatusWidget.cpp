#include "stdafx.h"
#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"
#include "../../utils/utils.h"
#include "MessageItem.h"
#include "MessageStatusWidget.h"
#include "../../theme_settings.h"

namespace
{
    int32_t getStatusIconBottomPadding(const Themes::PixmapResourceId resId);

    const QFont& getTimeFont();

    int32_t getTimeStatusMargin();
}

namespace Ui
{

    int32_t MessageStatusWidget::getMaxWidth(const bool withStatusIcon)
    {
        return (
            withStatusIcon ?
                Utils::scale_value(54) :
                Utils::scale_value(30)
        );
    }

    MessageStatusWidget::MessageStatusWidget(HistoryControlPageItem *messageItem)
        : QWidget(messageItem)
        , IsDeliveredToClient_(false)
        , IsDeliveredToServer_(false)
        , StatusIcon_(Themes::PixmapResourceId::Invalid)
        , IsMessageBubbleVisible_(true)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void MessageStatusWidget::setDeliveredToClient()
    {
        if (IsDeliveredToClient_)
        {
            return;
        }

        IsDeliveredToServer_ = true;

        IsDeliveredToClient_ = true;

        StatusIcon_ = Themes::PixmapResourceId::ContactListReadMark;

        updateGeometry();

        update();
    }

    void MessageStatusWidget::setDeliveredToServer()
    {
        if (IsDeliveredToServer_)
        {
            return;
        }

        IsDeliveredToServer_ = true;

        StatusIcon_ = Themes::PixmapResourceId::ContactListDeliveredMark;

        updateGeometry();

        update();
    }

    void MessageStatusWidget::setTime(const int32_t timestamp)
    {
        TimeText_ =
            QDateTime::fromTime_t(timestamp)
                .time()
                .toString("HH:mm");

        QFontMetrics m(getTimeFont());

        TimeTextSize_ = m.boundingRect(TimeText_).size();
        assert(TimeTextSize_.isValid());

        updateGeometry();

        update();
    }

    QSize MessageStatusWidget::sizeHint() const
    {
        assert(!TimeTextSize_.isEmpty());

        const auto hasStatusIcon = (StatusIcon_ != Themes::PixmapResourceId::Invalid);

        const auto width = getMaxWidth(hasStatusIcon);

        auto height = TimeTextSize_.height();

        if (hasStatusIcon)
        {
            const auto statusPixmap = Themes::GetPixmap(StatusIcon_);
            assert(statusPixmap);

            auto statusHeight = statusPixmap->GetHeight();
            statusHeight -= getStatusIconBottomPadding(StatusIcon_);

            height = std::max(height, statusHeight);
        }

        assert(width > 0);
        assert(height > 0);

        return QSize(width, height);
    }

    void MessageStatusWidget::paintEvent(QPaintEvent *)
    {
        QPainter p(this);

        const auto height = sizeHint().height();

        auto cursorX = 0;

        if (StatusIcon_ != Themes::PixmapResourceId::Invalid)
        {
            const auto statusPixmap = Themes::GetPixmap(StatusIcon_);
            assert(statusPixmap);

            auto statusY = height;
            statusY -= statusPixmap->GetHeight();
            statusY += getStatusIconBottomPadding(StatusIcon_);

            statusPixmap->Draw(p, cursorX, statusY);

            cursorX += getTimeStatusMargin();
            cursorX += statusPixmap->GetWidth();
        }

        p.setFont(getTimeFont());
        p.setPen(getTimeColor());

        const auto textBaseline = height;

        assert(!TimeText_.isEmpty());
        p.drawText(cursorX, textBaseline, TimeText_);
    }
    
    QColor MessageStatusWidget::getTimeColor() const
    {
        auto curTheme = Ui::get_qt_theme_settings()->themeForContact(aimId_);
        if (curTheme)
        {
            if (IsMessageBubbleVisible_)
            {
                if (IsOutgoing_)
                {
                    return curTheme->outgoing_bubble_.time_color_;
                }
                else
                {
                    return curTheme->incoming_bubble_.time_color_;
                }
            }
            else
            {
                return curTheme->preview_stickers_.time_color_;
            }
        }
        return QColor(0x979797);
    }

    void MessageStatusWidget::setMessageBubbleVisible(const bool _visible)
    {
        IsMessageBubbleVisible_ = _visible;
    }
}

namespace
{
    int32_t getStatusIconBottomPadding(const Themes::PixmapResourceId /*resId*/)
    {
        return Utils::scale_value(3);
    }

    const QFont& getTimeFont()
    {
        static QFont font(
            Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI)
        );

        font.setPixelSize(
            Utils::scale_value(12)
        );

        return font;
    }

    int32_t getTimeStatusMargin()
    {
        return Utils::scale_value(8);
    }
}