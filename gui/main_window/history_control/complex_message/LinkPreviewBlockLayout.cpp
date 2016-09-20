#include "stdafx.h"

#include "../../../controls/TextEditEx.h"
#include "../../../fonts.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text.h"
#include "../../../utils/profiling/auto_stop_watch.h"
#include "../../../utils/utils.h"

#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"

#include "LinkPreviewBlock.h"
#include "Style.h"

#include "LinkPreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    int32_t evaluateFullTextHeight(const Utils::TextHeightMetrics &titleMetrics, const Utils::TextHeightMetrics &annotationMetrics);

    int32_t getAnnotationPlaceholderHeight();

    int32_t getAnnotationPlaceholderRightPadding();

    int32_t getAnnotationTopPadding();

    QSize getImagePreloaderSize();

    int32_t getLinesNumber(const TextEditEx &textControl);

    int32_t getMessageTopPadding();

    int32_t getPlaceholderMinRightPadding();

    int32_t getPlaceholderVisibilityThreshold();

    int32_t getPreviewRightPadding();

    int32_t getSiteNamePlaceholderHeight();

    int32_t getSiteNamePlaceholderTopMargin();

    int32_t getTitlePlaceholderHeight();
}

LinkPreviewBlockLayout::LinkPreviewBlockLayout()
{
}

IItemBlockLayout* LinkPreviewBlockLayout::asBlockLayout()
{
    return this;
}

QLayout* LinkPreviewBlockLayout::asQLayout()
{
    return this;
}

void LinkPreviewBlockLayout::cutTextByPreferredSize(
    const QString &title,
    const QString &annotation,
    const QRect &blockLtr,
    Out QString &cutTitle,
    Out QString &cutAnnotation) const
{
    assert(blockLtr.width() > 0);

    Out cutTitle = title;
    Out cutAnnotation = annotation;

    const auto previewContentLtr = evaluatePreviewContentLtr(blockLtr);

    const auto previewImageRect = evaluatePreviewImageRect(previewContentLtr, false);
    assert(!previewImageRect.isEmpty());

    const auto titleLtr = evaluateTitleLtr(
        previewContentLtr,
        previewImageRect.width(),
        false);

    const auto textWidth = titleLtr.width();

    const auto &titleFont = getTitleFont();
    const QFontMetrics titleFontMetrics(titleFont);

    const auto &annotationFont = getAnnotationFont();
    const QFontMetrics annotationFontMetrics(annotationFont);

    const auto initialTitleHeightMetrics = Utils::evaluateTextHeightMetrics(title, textWidth, titleFontMetrics);
    const auto initialAnnotationHeightMetrics = Utils::evaluateTextHeightMetrics(annotation, textWidth, annotationFontMetrics);

    auto titleHeightMetrics = initialTitleHeightMetrics;
    auto annotationHeightMetrics = initialAnnotationHeightMetrics;

    for(;;)
    {
        const auto fullTextHeight = evaluateFullTextHeight(titleHeightMetrics, annotationHeightMetrics);

        const auto overwhelmingY = (fullTextHeight - previewImageRect.height());

        const auto hasEnoughPlaceForText = (overwhelmingY <= 0);
        if (hasEnoughPlaceForText)
        {
            break;
        }

        const auto ANNOTATION_LINES_MIN = 3;
        const auto cutAnnotation = (annotationHeightMetrics.getHeightLines() > ANNOTATION_LINES_MIN);

        if (cutAnnotation)
        {
            annotationHeightMetrics = annotationHeightMetrics.linesNumberChanged(-1);
            continue;
        }

        const auto TITLE_LINES_MIN = 2;
        const auto cutTitle = (titleHeightMetrics.getHeightLines() > TITLE_LINES_MIN);

        if (cutTitle)
        {
            titleHeightMetrics = titleHeightMetrics.linesNumberChanged(-1);
            continue;
        }

        break;
    }

    static const QString ELLIPSIS("...");

    Out cutTitle = Utils::limitLinesNumber(
        title,
        textWidth,
        titleFontMetrics,
        titleHeightMetrics.getHeightLines(),
        ELLIPSIS);

    Out cutAnnotation = Utils::limitLinesNumber(
        annotation,
        textWidth,
        annotationFontMetrics,
        annotationHeightMetrics.getHeightLines(),
        ELLIPSIS);
}

QFont LinkPreviewBlockLayout::getAnnotationFont() const
{
    return MessageStyle::getTextFont();
}

QRect LinkPreviewBlockLayout::getAnnotationRect() const
{
    return AnnotationGeometry_;
}

const IItemBlockLayout::IBoxModel& LinkPreviewBlockLayout::getBlockBoxModel() const
{
    static const BoxModel boxModel(
        true,
        Style::getDefaultBlockBubbleMargins());

    return boxModel;
}

QSize LinkPreviewBlockLayout::getMaxPreviewSize() const
{
    return QSize(
        Utils::scale_value(180),
        Style::getImageHeightMax());
}

QRect LinkPreviewBlockLayout::getFaviconImageRect() const
{
    return FaviconImageRect_;
}

QRect LinkPreviewBlockLayout::getPreviewImageRect() const
{
    return PreviewImageRect_;
}

QPoint LinkPreviewBlockLayout::getSiteNamePos() const
{
    return SiteNameGeometry_.bottomLeft();
}

QRect LinkPreviewBlockLayout::getSiteNameRect() const
{
    return SiteNameGeometry_;
}

QFont LinkPreviewBlockLayout::getTitleFont() const
{
    return Fonts::appFontScaled(21);
}

QRect LinkPreviewBlockLayout::getTitleRect() const
{
    return TitleGeometry_;
}

bool LinkPreviewBlockLayout::isAnnotationVisible() const
{
    return true;
}

QRect LinkPreviewBlockLayout::evaluateAnnotationLtr(const QRect &titleGeometry) const
{
    auto annotationWidth = titleGeometry.width();

    const auto &block = *blockWidget<LinkPreviewBlock>();

    if (block.isInPreloadingState())
    {
        annotationWidth -= getAnnotationPlaceholderRightPadding();

        if (annotationWidth < 0)
        {
            annotationWidth = titleGeometry.width();
        }
    }

    QRect annotationLtr(
        titleGeometry.left(),
        titleGeometry.bottom() + 1,
        annotationWidth,
        0);

    const auto hasTitle = !titleGeometry.isEmpty();
    if (hasTitle)
    {
        annotationLtr.translate(0, getAnnotationTopPadding());
    }

    assert(annotationLtr.isEmpty());
    return annotationLtr;
}

QRect LinkPreviewBlockLayout::evaluateFaviconImageRect(const QRect &bottomTextBlockGeometry) const
{
    QRect faviconGeometry(
        bottomTextBlockGeometry.left(),
        bottomTextBlockGeometry.bottom() + 1,
        0,
        0);

    const auto hasTextAbove = !bottomTextBlockGeometry.isEmpty();
    if (hasTextAbove)
    {
        faviconGeometry.translate(0, Style::getFaviconTopPadding());
    }

    const auto &block = *blockWidget<LinkPreviewBlock>();

    const auto &faviconSize = block.getFaviconSizeUnscaled();
    faviconGeometry.setSize(faviconSize);

    return Utils::scale_value(faviconGeometry);
}

QRect LinkPreviewBlockLayout::evaluatePreviewContentLtr(const QRect &widgetLtr) const
{
    assert(widgetLtr.width() > 0);

    const auto previewContentWidth = widgetLtr.width();

    const QRect previewContentLtr(
        widgetLtr.topLeft(),
        QSize(previewContentWidth, 0));

    return previewContentLtr;
}

QRect LinkPreviewBlockLayout::evaluatePreviewImageRect(const QRect &previewContentLtr, const bool isInPreloadingState) const
{
    assert(previewContentLtr.isEmpty());

    const auto &block = *blockWidget<LinkPreviewBlock>();

    const auto &previewImageSize = (
        isInPreloadingState ?
            getImagePreloaderSize() :
            block.getPreviewImageSize());

    const auto isEnoughPlace = (previewContentLtr.width() > getPlaceholderVisibilityThreshold());
    const auto hidePreviewImage = (previewImageSize.isEmpty() || !isEnoughPlace);
    if (hidePreviewImage)
    {
        return QRect(
            previewContentLtr.left(),
            previewContentLtr.top(),
            0,
            0);
    }

    const QRect previewImageRect(previewContentLtr.topLeft(), previewImageSize);
    assert(!previewImageRect.isEmpty());

    return previewImageRect;

}

QRect LinkPreviewBlockLayout::evaluateSiteNameGeometry(const QRect &bottomTextBlockGeometry, const QRect &faviconGeometry, const QRect &titleGeometry) const
{
    auto siteNameY = (bottomTextBlockGeometry.bottom() + 1);

    const auto hasTextAbove = !bottomTextBlockGeometry.isEmpty();
    if (hasTextAbove)
    {
        siteNameY += Style::getSiteNameTopPadding();
    }

    auto siteNameX = (faviconGeometry.right() + 1);

    const auto hasFavicon = !faviconGeometry.isEmpty();
    if (hasFavicon)
    {
        siteNameX += Style::getSiteNameLeftPadding();
    }

    const auto &block = *blockWidget<LinkPreviewBlock>();

    QSize siteNameSize(0, 0);

    const auto &siteName = block.SiteName_;
    if (block.isInPreloadingState())
    {
        assert(titleGeometry.width() > 0);

        siteNameSize = QSize(
            titleGeometry.width(),
            getSiteNamePlaceholderHeight());

        siteNameY = (
            bottomTextBlockGeometry.bottom() + 1 +
            getSiteNamePlaceholderTopMargin());
    }
    else if (!siteName.isEmpty())
    {
        QFontMetrics m(block.SiteNameFont_);

        siteNameSize = m.boundingRect(siteName).size();

        if (hasTextAbove)
        {
            siteNameY -= siteNameSize.height();
        }
    }

    const QPoint leftTop(siteNameX, siteNameY);

    return QRect(leftTop, siteNameSize);
}

QRect LinkPreviewBlockLayout::evaluateTitleLtr(const QRect &previewContentLtr, const int32_t previewWidth, const bool isPlaceholder) const
{
    assert((previewWidth == 0) || (previewWidth > 1));
    assert(previewContentLtr.isEmpty());

    auto titleLeft = previewContentLtr.left();

    const auto hasPreview = (previewWidth > 0);
    if (hasPreview)
    {
        titleLeft += previewWidth;
        titleLeft += getPreviewRightPadding();
    }

    const auto minTitleWidth = (MessageStyle::getTextWidthStep() * 2);

    const auto exactTitleWidth = (previewContentLtr.right() - titleLeft);
    if (exactTitleWidth <= minTitleWidth)
    {
        return QRect();
    }

    const auto roundedTitleWidth =
        isPlaceholder ?
            exactTitleWidth :
            MessageStyle::roundTextWidthDown(exactTitleWidth);

    QRect titleLtr;
    titleLtr.setLeft(titleLeft);
    titleLtr.setTop(previewContentLtr.top());
    titleLtr.setWidth(roundedTitleWidth);
    titleLtr.setHeight(0);

    assert(titleLtr.isEmpty());
    return titleLtr;
}

int32_t LinkPreviewBlockLayout::evaluateWidgetHeight(
    const QRect &previewImageGeometry,
    const QRect &faviconGeometry,
    const QRect &siteNameGeometry)
{
    const auto &block = *blockWidget<LinkPreviewBlock>();

    auto bottom = (previewImageGeometry.bottom() + 1);
    bottom = std::max(bottom, faviconGeometry.bottom() + 1);

    auto siteNameBottom = (siteNameGeometry.bottom() + 1);

    const auto hasSiteName = !siteNameGeometry.isEmpty();
    const auto applyBaselineFix = (hasSiteName && !block.isInPreloadingState());
    if (applyBaselineFix)
    {
        // an approximation of the gap between font baseline and font bottom line
        siteNameBottom += (siteNameGeometry.height() / 3);
    }

    bottom = std::max(bottom, siteNameBottom);

    const auto contentHeight = bottom;
    assert(contentHeight >= 0);

    const auto height = std::max(MessageStyle::getMinBubbleHeight(), contentHeight);

    return height;
}

QRect LinkPreviewBlockLayout::setAnnotationGeometry(const QRect &ltr)
{
    assert(ltr.isEmpty());

    const auto &block = *blockWidget<LinkPreviewBlock>();

    if (block.isInPreloadingState())
    {
        const QSize placeholderSize(
            ltr.width(),
            getAnnotationPlaceholderHeight());

        QRect preloaderRect(
            ltr.topLeft(),
            placeholderSize);

        return preloaderRect;
    }

    if (!block.Annotation_)
    {
        return ltr;
    }

    auto &annotation = *block.Annotation_;

    const auto textWidth = ltr.width();
    assert(textWidth > 0);

    const auto widthChanged = (textWidth != AnnotationGeometry_.width());
    if (widthChanged)
    {
        annotation.setFixedWidth(textWidth);
        annotation.document()->setTextWidth(textWidth);
    }

    const auto textX = ltr.left();
    assert(textX >= 0);

    const auto textY = ltr.top();
    assert(textY >= 0);

    const QRect annotationGeometry(
        textX,
        textY,
        textWidth,
        annotation.getTextHeight());

    const auto geometryChanged = (annotationGeometry != AnnotationGeometry_);
    if (geometryChanged)
    {
        annotation.setGeometry(annotationGeometry);

        AnnotationGeometry_ = annotationGeometry;
    }

    return annotationGeometry;
}

QSize LinkPreviewBlockLayout::setBlockGeometryInternal(const QRect &widgetLtr)
{
    assert(widgetLtr.height() >= 0);

    const auto enoughSpace = (widgetLtr.width() > 0);
    if (!enoughSpace)
    {
        return QSize();
    }

    const auto previewContentLtr = evaluatePreviewContentLtr(widgetLtr);

    const auto &block = *blockWidget<LinkPreviewBlock>();

    PreviewImageRect_ = evaluatePreviewImageRect(previewContentLtr, block.isInPreloadingState());

    const auto titleLtr = evaluateTitleLtr(
        previewContentLtr,
        PreviewImageRect_.width(),
        block.isInPreloadingState());

    const auto enoughPlaceForTitle = (titleLtr.width() > 0);
    if (!enoughPlaceForTitle)
    {
        return QSize();
    }

    TitleGeometry_ = setTitleGeometry(titleLtr);

    const auto annotationLtr = evaluateAnnotationLtr(TitleGeometry_);

    AnnotationGeometry_ = setAnnotationGeometry(annotationLtr);

    const auto &bottomTextBlockGeometry = (
        AnnotationGeometry_.isEmpty() ? TitleGeometry_ : AnnotationGeometry_);

    FaviconImageRect_ = evaluateFaviconImageRect(bottomTextBlockGeometry);

    SiteNameGeometry_ = evaluateSiteNameGeometry(bottomTextBlockGeometry, FaviconImageRect_, TitleGeometry_);

    const auto blockHeight = evaluateWidgetHeight(
        PreviewImageRect_,
        FaviconImageRect_,
        SiteNameGeometry_);
    assert(blockHeight >= MessageStyle::getMinBubbleHeight());

    const QSize blockSize(widgetLtr.width(), blockHeight);

    return blockSize;
}

QRect LinkPreviewBlockLayout::setTitleGeometry(const QRect &ltr)
{
    assert(ltr.isEmpty());

    const auto &block = *blockWidget<LinkPreviewBlock>();

    if (block.isInPreloadingState())
    {
        QSize placeholderSize(
            ltr.width(),
            getTitlePlaceholderHeight());

        QRect preloaderGeometry(
            ltr.topLeft(),
            placeholderSize);

        return preloaderGeometry;
    }

    if (!block.Title_)
    {
        return ltr;
    }

    auto &title = *block.Title_;

    const auto textWidth = ltr.width();
    assert(textWidth > 0);

    const auto widthChanged = (textWidth != CurrentTitleGeometry_.width());
    if (widthChanged)
    {
        title.setFixedWidth(textWidth);
        title.document()->setTextWidth(textWidth);
    }

    const auto textX = ltr.left();
    assert(textX >= 0);

    const auto textY = ltr.top();
    assert(textY >= 0);

    const QRect titleGeometry(
        textX,
        textY,
        textWidth,
        title.getTextHeight());

    const auto geometryChanged = (titleGeometry != CurrentTitleGeometry_);
    if (geometryChanged)
    {
        title.setGeometry(titleGeometry);

        CurrentTitleGeometry_ = titleGeometry;
    }

    return titleGeometry;
}

namespace
{
    int32_t evaluateFullTextHeight(const Utils::TextHeightMetrics &titleMetrics, const Utils::TextHeightMetrics &annotationMetrics)
    {
        auto textHeight = titleMetrics.getHeightPx();

        const auto hasTitle = (textHeight > 0);
        if (hasTitle)
        {
            textHeight += getAnnotationTopPadding();
        }

        textHeight += annotationMetrics.getHeightPx();

        return textHeight;
    }

    int32_t getAnnotationPlaceholderHeight()
    {
        return Utils::scale_value(20);
    }

    int32_t getAnnotationPlaceholderRightPadding()
    {
        return Utils::scale_value(72);
    }

    int32_t getAnnotationTopPadding()
    {
        return Utils::scale_value(12);
    }

    QSize getImagePreloaderSize()
    {
        return Utils::scale_value(QSize(180, 76));
    }

    int32_t getLinesNumber(const TextEditEx &textControl)
    {
        const auto lineHeight = textControl.fontMetrics().height();

        const auto textHeight = textControl.getTextSize().height();

        const auto linesNumber = (int32_t)std::ceil((double)textHeight / (double)lineHeight);

        return linesNumber;
    }

    int32_t getMessageTopPadding()
    {
        return Utils::scale_value(4);
    }

    int32_t getPlaceholderMinRightPadding()
    {
        return Utils::scale_value(24);
    }

    int32_t getPlaceholderVisibilityThreshold()
    {
        return Utils::scale_value(360);
    }

    int32_t getPreviewRightPadding()
    {
        return Utils::scale_value(16);
    }

    int32_t getSiteNamePlaceholderHeight()
    {
        return Utils::scale_value(12);
    }

    int32_t getSiteNamePlaceholderTopMargin()
    {
        return Utils::scale_value(12);
    }

    int32_t getTitlePlaceholderHeight()
    {
        return Utils::scale_value(20);
    }

}

UI_COMPLEX_MESSAGE_NS_END

