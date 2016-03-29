#include "stdafx.h"

#include "../../cache/themes/themes.h"
#include "../../controls/DpiAwareImage.h"
#include "../../controls/TextEditEx.h"
#include "../../theme_settings.h"

#include "../../utils/log/log.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"

#include "ChatEventInfo.h"
#include "ChatEventItem.h"

namespace Ui
{

	namespace
	{
		qint32 getBubbleHorPadding();

		qint32 getBubbleRadius();

        qint32 getIconLeftPadding();

        int32_t getMinBubbleOffset();

		qint32 getTextHorPadding();

		qint32 getTextVertOffset();

        Utils::FontsFamily getWidgetFontFamily();

        int32_t getWidgetFontSize();

		qint32 getWidgetMinHeight();

		qint32 getIconRightPadding();
    }

    ChatEventItem::~ChatEventItem()
    {
    }

    ChatEventItem::ChatEventItem(const HistoryControl::ChatEventInfoSptr& eventInfo)
        : HistoryControlPageItem(nullptr)
        , EventInfo_(eventInfo)
        , TextWidget_(nullptr)
    {
    }

	ChatEventItem::ChatEventItem(QWidget *parent, const HistoryControl::ChatEventInfoSptr& eventInfo)
		: HistoryControlPageItem(parent)
		, EventInfo_(eventInfo)
		, TextWidget_(nullptr)
	{
		assert(EventInfo_);

        TextWidget_ = new TextEditEx(
            this,
            getWidgetFontFamily(),
            getWidgetFontSize(),
            QColor("#696969"),
            false,
            false
        );

        TextWidget_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        TextWidget_->setFrameStyle(QFrame::NoFrame);
        TextWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        TextWidget_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        TextWidget_->setOpenLinks(true);
        TextWidget_->setOpenExternalLinks(true);
        TextWidget_->setWordWrapMode(QTextOption::WordWrap);
        TextWidget_->setFocusPolicy(Qt::NoFocus);
        TextWidget_->setStyleSheet("background: transparent");
        TextWidget_->document()->setDocumentMargin(0);

        const auto &message = EventInfo_->formatEventText();
        Logic::Text4Edit(message, *TextWidget_, Logic::Text2DocHtmlMode::Escape, true, true);

        TextWidget_->setAlignment(Qt::AlignHCenter);
        TextWidget_->show();

        Icon_.reset(
            new DpiAwareImage(
			    EventInfo_->loadEventIcon(
				    getWidgetMinHeight()
			    )
            )
		);

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        updateTheme();
	}

	QString ChatEventItem::formatRecentsText() const
	{
		return EventInfo_->formatEventText();
	}

    int32_t ChatEventItem::evaluateFullIconWidth()
    {
        if (Icon_->isNull())
        {
            return 0;
        }

        auto fullIconWidth = getIconLeftPadding();
        fullIconWidth += Icon_->width();
        fullIconWidth += getIconRightPadding();

        return fullIconWidth;
    }

    int32_t ChatEventItem::evaluateTextHeight(const int32_t textWidth)
    {
        assert(textWidth > 0);

        auto &document = *TextWidget_->document();

        if (TextWidget_->width() != textWidth)
        {
            TextWidget_->setFixedWidth(textWidth);
            document.setTextWidth(textWidth);
        }

        return TextWidget_->getTextSize().height();
    }

    int32_t ChatEventItem::evaluateTextWidth(const int32_t widgetWidth)
    {
        assert(widgetWidth > 0);

        const auto maxBubbleWidth = (
            widgetWidth -
            getBubbleHorPadding() -
            getBubbleHorPadding()
        );

        const auto maxBubbleContentWidth = (
            maxBubbleWidth -
            getTextHorPadding() -
            getTextHorPadding()
        );

        const auto maxTextWidth = (
            maxBubbleContentWidth -
            evaluateFullIconWidth()
        );

        auto &document = *TextWidget_->document();

        if (TextWidget_->width() != maxTextWidth)
        {
            TextWidget_->setFixedWidth(maxTextWidth);
            document.setTextWidth(maxTextWidth);
        }

        const auto idealWidth = document.idealWidth();

        const auto widthInfidelityFix = (
            QFontMetrics(document.defaultFont()).averageCharWidth() * 2
        );

        const auto fixedWidth = (idealWidth + widthInfidelityFix);

        return fixedWidth;
    }

    void ChatEventItem::updateTheme()
    {
        const auto theme = get_qt_theme_settings()->themeForContact(aimId_);
        const auto textColor = theme->chat_event_.text_color_;

        auto palette = TextWidget_->palette();

        const auto textColorChanged = (
            palette.color(QPalette::Text) != textColor
        );
        if (!textColorChanged)
        {
            return;
        }

        palette.setBrush(QPalette::Background, QBrush(Qt::transparent));
        palette.setColor(QPalette::Text, textColor);
        TextWidget_->setPalette(palette);

        TextWidget_->document()->setDefaultFont(Utils::appFont(
            getWidgetFontFamily(),
            getWidgetFontSize()
        ));
    }

	void ChatEventItem::paintEvent(QPaintEvent*)
	{
		assert(!BubbleRect_.isEmpty());

		QPainter p(this);

		p.setRenderHint(QPainter::Antialiasing);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		p.setRenderHint(QPainter::TextAntialiasing);

		if (!Icon_->isNull())
		{
			const auto iconX = (BubbleRect_.left() + getIconLeftPadding());

			const auto iconY = (height() - Icon_->height()) / 2;

			Icon_->draw(p, iconX, iconY);
		}

        updateTheme();
	}

	void ChatEventItem::resizeEvent(QResizeEvent *event)
	{
        HistoryControlPageItem::resizeEvent(event);

        const auto &newSize = event->size();
        const auto newWidth = newSize.width();

        const auto &oldSize = event->oldSize();

        const auto sameWidth = (oldSize.width() == newWidth);
        if (sameWidth)
        {
            return;
        }

        // setup the text control and get it dimensions
        const auto textWidth = evaluateTextWidth(newWidth);
        const auto textHeight = evaluateTextHeight(textWidth);
        TextWidget_->setFixedSize(textWidth, textHeight);

        // evaluate bubble width

		auto bubbleWidth = 0;

        if (Icon_->isNull())
        {
            bubbleWidth += getTextHorPadding();
        }
        else
        {
            bubbleWidth += evaluateFullIconWidth();
        }

        bubbleWidth += textWidth;
        bubbleWidth += getTextHorPadding();

        // evaluate bubble height

        auto bubbleHeight = textHeight;
        bubbleHeight += getTextVertOffset();
        bubbleHeight += getTextVertOffset();

		BubbleRect_ = QRect(0, 0, bubbleWidth, bubbleHeight);
		BubbleRect_.moveCenter(QRect(0, 0, newWidth, bubbleHeight).center());

        // setup geometry

        setFixedSize(newWidth, bubbleHeight);

        auto textWidgetLeft = BubbleRect_.left();

        if (Icon_->isNull())
        {
            textWidgetLeft += getTextHorPadding();
        }
        else
        {
            textWidgetLeft += evaluateFullIconWidth();
        }

        TextWidget_->move(
            textWidgetLeft,
            BubbleRect_.top() + getTextVertOffset()
        );
	}

	namespace
	{
		qint32 getBubbleHorPadding()
		{
			return Utils::scale_value(12);
		}

		qint32 getBubbleRadius()
		{
			return Utils::scale_value(10);
		}

        int32_t getMinBubbleOffset()
        {
            return Utils::scale_value(24);
        }

		qint32 getTextHorPadding()
		{
			return Utils::scale_value(12);
		}

		qint32 getTextVertOffset()
		{
			return Utils::scale_value(6);
		}

        Utils::FontsFamily getWidgetFontFamily()
        {
            return Utils::FontsFamily::SEGOE_UI_SEMIBOLD;
        }

        int32_t getWidgetFontSize()
        {
            return (Utils::scale_value(12) * qApp->devicePixelRatio());
        }

		qint32 getWidgetMinHeight()
		{
			return Utils::scale_value(22);
		}

		qint32 getIconRightPadding()
		{
			return Utils::scale_value(4);
		}

		qint32 getIconLeftPadding()
		{
			return Utils::scale_value(6);
		}

	}

};