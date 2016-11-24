#include "stdafx.h"

#include "../../../fonts.h"
#include "../../../utils/utils.h"

#include "Style.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace Style
{

    int32_t getBlocksSeparatorVertMargins()
    {
        return Utils::scale_value(16);
    }

    const QPen& getBlocksSeparatorPen()
    {
        static const QPen separatorPen(
            QBrush(QColor(0x57, 0x9e, 0x1c)),
            (qreal)Utils::scale_value(1));

        return separatorPen;
    }

    const QPen& getQuoteSeparatorPen()
    {
        static const QPen separatorPen(
            QBrush(QColor(0x57, 0x9e, 0x1c)),
            (qreal)Utils::scale_value(2));

        return separatorPen;
    }

    const QMargins& getDefaultBlockBubbleMargins()
    {
        static const QMargins margins(
            Utils::scale_value(16),
            Utils::scale_value(16),
            Utils::scale_value(8),
            Utils::scale_value(16));

        return margins;
    }

    bool isBlocksGridEnabled()
    {
        if (!build::is_debug())
        {
            return false;
        }

        return false;
    }

    int32_t getFaviconTopPadding()
    {
        return Utils::scale_value(8);
    }

    QPen getFileSharingFramePen()
    {
        const auto alpha = ((40 * 255) / 100);
        const QColor color(0x69, 0x69, 0x69, alpha);
        const QBrush brush(color);
        const auto width = Utils::scale_value(2);
        return QPen(brush, width);
    }

    int32_t getImageHeightMax()
    {
        return Utils::scale_value(400);
    }

    int32_t getImageWidthMax()
    {
        return Utils::scale_value(480);
    }

    int32_t getMinFileSize4ProgressBar()
    {
        return (3 * 1024 * 1024);
    }

    int32_t getPttBubbleHeaderHeight()
    {
        return Utils::scale_value(56);
    }

    int32_t getShareButtonLeftMargin()
    {
        return Utils::scale_value(10);
    }

    int32_t getSiteNameLeftPadding()
    {
        return Utils::scale_value(4);
    }

    int32_t getSiteNameTopPadding()
    {
        return Utils::scale_value(22);
    }

    int32_t getQuoteAvatarOffset()
    {
        return Utils::scale_value(25);
    }

    int32_t getQuoteOffsetLeft()
    {
        return Utils::scale_value(11);
    }
    
    int32_t getQuoteOffsetTop()
    {
        return Utils::scale_value(12);
    }
    
    int32_t getQuoteOffsetBottom()
    {
        return Utils::scale_value(10);
	}
    
    int32_t getQuoteSpacing()
    {
        return Utils::scale_value(2);
    }
    
    int32_t getFirstQuoteOffset()
    {
        return Utils::scale_value(6);
    }
    
    int32_t getTextQuoteOffset()
    {
        return Utils::scale_value(4);
    }

    int32_t getMaxImageWidthInQuote()
    {
        return Utils::scale_value(200);
    }

    int32_t getQuoteLineTopMargin()
    {
        return Utils::scale_value(5);
    }

    int32_t getQuoteAvatarOffsetTop()
    {
        return Utils::scale_value(3);
    }

    int32_t getQuoteLineBottomMargin()
    {
        return Utils::scale_value(4);
    }

    int32_t getForwardLabelBottomMargin()
    {
        return Utils::scale_value(6);
    }

    int32_t getForwardIconOffset()
    {
        return Utils::scale_value(5);
    }

    int32_t getLineOffset()
    {
        return Utils::scale_value(7);
    }

    int32_t getForwardLabelOffset()
    {
        return Utils::scale_value(4);
    }

    int32_t getQuoteBlockSpacing()
    {
        return Utils::scale_value(5);
    }

    int32_t getQuoteBottomSpace()
    {
        return Utils::scale_value(3);
    }

    QFont getYoutubeTitleFont()
    {
        return Fonts::appFontScaled(17);
    }

}

UI_COMPLEX_MESSAGE_NS_END