#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace Style
{

    int32_t getBlocksSeparatorVertMargins();

    const QPen& getBlocksSeparatorPen();

    const QPen& getQuoteSeparatorPen();

    const QMargins& getDefaultBlockBubbleMargins();

    bool isBlocksGridEnabled();

    int32_t getFaviconTopPadding();

    QPen getFileSharingFramePen();

    int32_t getImageHeightMax();

    int32_t getImageWidthMax();

    int32_t getMinFileSize4ProgressBar();

    int32_t getPttBubbleHeaderHeight();

    int32_t getShareButtonLeftMargin();

    int32_t getSiteNameLeftPadding();

    int32_t getSiteNameTopPadding();

    int32_t getQuoteOffsetLeft();
    
    int32_t getQuoteOffsetTop();
    
    int32_t getQuoteOffsetBottom();
    
    int32_t getQuoteSpacing();
    
    int32_t getFirstQuoteOffset();
    
    int32_t getTextQuoteOffset();

    int32_t getMaxImageWidthInQuote();

    int32_t getQuoteLineTopMargin();

    int32_t getQuoteLineBottomMargin();

    int32_t getForwardLabelBottomMargin();

    int32_t getForwardIconOffset();

    QFont getYoutubeTitleFont();
}

UI_COMPLEX_MESSAGE_NS_END