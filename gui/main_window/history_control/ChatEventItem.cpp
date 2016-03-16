#include "stdafx.h"
#include "../../utils/utils.h"
#include "ChatEventInfo.h"
#include "ChatEventItem.h"
#include "../../cache/themes/themes.h"
#include "../../theme_settings.h"

namespace Ui
{

	namespace
	{
		qint32 getBubbleRightPadding();

		qint32 getBubbleRadius();

        int32_t getMinBubbleOffset();

		qint32 getTextLeftPadding();

		qint32 getTextVertOffset();

		const QFont& getWidgetFont();
        
        const Utils::FontsFamily getWidgetFontFamily();

		qint32 getWidgetHeight();

		qint32 getIconLeftPadding();

		qint32 getIconRightPadding();

		qint32 getWidgetVertPadding();
	}

    ChatEventItem::ChatEventItem(const HistoryControl::ChatEventInfoSptr& eventInfo)
        : HistoryControlPageItem(nullptr)
        , EventInfo_(eventInfo)
        , CachedTextWidth_(0)
        , emojer_(nullptr, getWidgetFontFamily(), Utils::scale_value(12) * qApp->devicePixelRatio(), QColor("#696969"), -1)
    {
    }

	ChatEventItem::ChatEventItem(QWidget *parent, const HistoryControl::ChatEventInfoSptr& eventInfo)
		: HistoryControlPageItem(parent)
		, EventInfo_(eventInfo)
		, CachedTextWidth_(0)
		, emojer_(nullptr, getWidgetFontFamily(), Utils::scale_value(12) * qApp->devicePixelRatio(), QColor("#696969"), -1)
	{
		assert(EventInfo_);

        emojer_.setText(eventInfo->formatEventText());

        auto palette = QPalette(emojer_.palette());
        palette.setBrush(QPalette::ColorRole::Background, QBrush(QColor(Qt::transparent)));
        emojer_.setPalette(palette);

		Icon_ = DpiAwareImage(
			EventInfo_->loadEventIcon(
				getWidgetHeight()
			)
		);

        setFixedHeight(sizeHint().height());
	}

	QString ChatEventItem::formatRecentsText() const
	{
		return EventInfo_->formatEventText();
	}

    QSize ChatEventItem::sizeHint() const
    {
        return QSize(
            0,
            getWidgetHeight() + (getWidgetVertPadding() * 2)
        );
    }

	void ChatEventItem::paintEvent(QPaintEvent*)
	{
		assert(!BubbleRect_.isEmpty());

		QPainter p(this);

		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.setRenderHint(QPainter::TextAntialiasing);

		p.setPen(Qt::NoPen);
        
        auto theme = get_qt_theme_settings()->themeForContact(aimId_);
        QColor backgroundColor = theme->chat_event_.bg_color_;
        QColor textColor = theme->chat_event_.text_color_;
        
		p.setBrush(QBrush(backgroundColor));

		p.drawRoundedRect(BubbleRect_, getBubbleRadius(), getBubbleRadius());

		p.setPen(Qt::white);
		p.setBrush(Qt::NoBrush);
		p.setFont(getWidgetFont());
        
        emojer_.setColor(textColor);

		auto cursorX = BubbleRect_.left();

		if (Icon_)
		{
			cursorX += getIconLeftPadding();

			const auto iconY = (height() - Icon_.height()) / 2;

			Icon_.draw(p, cursorX, iconY);

			cursorX += Icon_.width();
			cursorX += getIconRightPadding();
		}
		else
		{
			cursorX += getTextLeftPadding();
        }

        auto pm = emojer_.grab();
        pm.setDevicePixelRatio(qApp->devicePixelRatio());
        p.drawPixmap(cursorX, getTextVertOffset(), pm);
	}

	void ChatEventItem::resizeEvent(QResizeEvent *event)
	{
        HistoryControlPageItem::resizeEvent(event);

        const auto &newSize = event->size();

		setFixedWidth(newSize.width());

		QFontMetrics m(getWidgetFont());

		assert(CachedTextWidth_ >= 0);
		if (CachedTextWidth_ == 0)
		{
			const auto &text = EventInfo_->formatEventText();
			assert(!text.isEmpty());

			// a very slow operation, therefore cache the results.
			CachedTextWidth_ = m.boundingRect(text).width();
			assert(CachedTextWidth_ > 0);

            CachedTextWidth_ += m.width(QChar(' ')); // workaround for TextEmojiWidget (margins?) bug
		}

		auto bubbleWidth = CachedTextWidth_ + getBubbleRightPadding();

		if (Icon_)
		{
			bubbleWidth += getIconLeftPadding();
			bubbleWidth += Icon_.width();
			bubbleWidth += getIconRightPadding();
		}
		else
		{
			bubbleWidth += getTextLeftPadding();
		}

        const auto maxBubbleWidth = (newSize.width() - (getMinBubbleOffset() * 2));
        bubbleWidth = std::min(maxBubbleWidth, bubbleWidth);

		BubbleRect_ = QRect(0, 0, bubbleWidth, getWidgetHeight());
		BubbleRect_.moveCenter(rect().center());

        auto textWidth = BubbleRect_.width();

        if (Icon_)
        {
            textWidth -= getIconLeftPadding();
            textWidth -= Icon_.width();
            textWidth -= getIconRightPadding();
        }
        else
        {
            textWidth -= getTextLeftPadding();
        }

        textWidth -=  getBubbleRightPadding();

        emojer_.setFixedWidth(textWidth * qApp->devicePixelRatio());
	}

	namespace
	{
		qint32 getBubbleRightPadding()
		{
			return Utils::scale_value(12);
		}

		qint32 getBubbleRadius()
		{
			return Utils::scale_value(10);
		}

        int32_t getMinBubbleOffset()
        {
            return Utils::scale_value(80);
        }

		qint32 getTextLeftPadding()
		{
			return Utils::scale_value(12);
		}

		qint32 getTextVertOffset()
		{
			return Utils::scale_value(6);
		}

		const QFont& getWidgetFont()
		{
            static QFont font(
                Utils::appFont(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, Utils::scale_value(12))
            );
            
            return font;
		}
        
        const Utils::FontsFamily getWidgetFontFamily()
        {
            return Utils::FontsFamily::SEGOE_UI_SEMIBOLD;
        }

		qint32 getWidgetHeight()
		{
			return Utils::scale_value(20);
		}

		qint32 getIconRightPadding()
		{
			return Utils::scale_value(4);
		}

		qint32 getIconLeftPadding()
		{
			return Utils::scale_value(6);
		}

		qint32 getWidgetVertPadding()
		{
			return Utils::scale_value(4);
		}
	}

};